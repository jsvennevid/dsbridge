/*

Copyright 2009 Jesper Svennevid

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "HttpServer.h"
#include "Notify.h"
#include "ExceptionHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace dsbridge
{

HttpServer::HttpServer()
: m_thread(0)
, m_socket(-1)
, m_clients(0)
, m_clientCount(0)
, m_running(false)
, m_lastAnnounce(time(0))
{}

HttpServer::~HttpServer()
{
}

bool HttpServer::create()
{
	InitializeCriticalSection(&m_cs);

	if (!m_buffer.create(100 * 1024))
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not create ringbuffer");
		return false;
	}

	m_thread = CreateThread(0, 0, threadEntry, this, CREATE_SUSPENDED, 0);
	if (!m_thread)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not create thread");
		return false;
	}

	m_running = true;
	ResumeThread(m_thread);

	return true;
}

void HttpServer::destroy()
{
	m_running = false;
	SleepEx(10, FALSE);

	m_thread = 0;
}

void HttpServer::write(const void* buffer, size_t count)
{
	EnterCriticalSection(&m_cs);
	do
	{
		if (m_buffer.left() < count)
		{
			m_buffer.seek((m_buffer.left() + m_buffer.written()) / 2);
		}

		m_buffer.write(buffer, count);
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

DWORD WINAPI HttpServer::threadEntry(LPVOID parameters)
{
	__try
	{
		HttpServer* server = static_cast<HttpServer*>(parameters);

		if (!server->initialize())
		{
			return 0;
		}

		while (server->run())
		{
			SleepEx(1, TRUE);
		}

		server->shutdown();

		Notify::update(Notify::HttpServer, Notify::Info, "Stopped");
	}
	__except(ExceptionHandler::filter("HttpServer", GetExceptionInformation()))
	{
	}
	return 0;
}

bool HttpServer::initialize()
{
	m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not create socket");
		return false;
	}

	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.S_un.S_addr = INADDR_ANY;
	saddr.sin_port = htons(8124);
	if (::bind(m_socket, reinterpret_cast<SOCKADDR*>(&saddr), sizeof(saddr)) < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not bind to port %d", ntohs(saddr.sin_port));
		return false;
	}

	BOOL reuse = TRUE;
	if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse)) < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not set SO_REUSEADDR");
		return false;
	}

	ULONG nonblock = 1;
	if (::ioctlsocket(m_socket, FIONBIO, &nonblock) < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not set server to non-blocking");
		return false;
	}

	if (::listen(m_socket, 1) < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not start listening to port");
		return false;
	}

	Notify::update(Notify::HttpServer, Notify::Info, "Listening on port %d", ntohs(saddr.sin_port));
	return true;
}

bool HttpServer::run()
{
	fd_set rfds;
	fd_set wfds;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	FD_SET(m_socket, &rfds);

	time_t newAnnounce = time(0);
	if ((newAnnounce - m_lastAnnounce) > 5)
	{
		Notify::setConnected(m_clientCount > 0);
		Notify::update(Notify::HttpServer, Notify::Info, "http://localhost:%d/", 8124);
		m_lastAnnounce = newAnnounce;
	}

	for (unsigned int i = 0; i < m_clientCount; ++i)
	{
		Client& client = m_clients[i];
		switch (client.m_state)
		{
			case Header:
			{
				FD_SET(client.m_socket, &rfds);
			}
			break;

			case Streaming:
			case Dead:
			{
				FD_SET(client.m_socket, &wfds);
			}
			break;
		}
	}

	timeval timeout = {0};
	int fd = ::select(0, &rfds, &wfds, 0, &timeout);
	if (fd < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "select() failed - %d", fd);
		return false;
	}
	else if (!fd)
	{
		return m_running;
	}

	for (unsigned int i = 0; i < m_clientCount; ++i)
	{
		Client& client = m_clients[i];

		switch (client.m_state)
		{
			case Header:
			{
				if (!FD_ISSET(client.m_socket, &rfds))
				{
					break;
				}

				int result = ::recv(client.m_socket, client.m_buffer + client.m_bufferSize, int(sizeof(client.m_buffer) - client.m_bufferSize), 0);
				if (result <= 0)
				{
					client.m_state = Dead;
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				client.m_bufferSize += result;

				processHeader(client);
			}
			break;

			case Streaming:
			case Dead:
			{
				if (!FD_ISSET(client.m_socket, &wfds))
				{
					break;
				}

				if (client.m_state == Streaming)
				{
					processStreaming(client);
				}

				int result = ::send(client.m_socket, client.m_buffer + client.m_bufferOffset, int(client.m_bufferSize - client.m_bufferOffset), 0);
				if (result == SOCKET_ERROR)
				{
					client.m_state = Dead;	
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				client.m_bufferOffset += result;
			}
			break;
		}
	}

	for (unsigned int i = 0; i < m_clientCount;)
	{
		Client& client = m_clients[i];
		if ((client.m_state != Dead) || (client.m_bufferSize != client.m_bufferOffset))
		{
			++i;
			continue;
		}

		::closesocket(client.m_socket);

		::memmove(&client, &client + 1, sizeof(Client) * (m_clientCount - (i+1)));
		--m_clientCount;
	}

	if (FD_ISSET(m_socket, &rfds))
	{
		SOCKADDR_IN saddr;
		int saddrlen = sizeof(saddr);
		SOCKET clientSocket = ::accept(m_socket, reinterpret_cast<SOCKADDR*>(&saddr), &saddrlen);
		if (clientSocket != INVALID_SOCKET)
		{
			Client* clients = new Client[m_clientCount + 1];
			if (m_clientCount > 0)
			{
				memcpy(clients, m_clients, m_clientCount * sizeof(Client));
				delete [] m_clients;
			}

			clients[m_clientCount].m_socket = clientSocket;
			clients[m_clientCount].m_state = Header;

			m_clients = clients;
			++ m_clientCount;
		}
	}

	return m_running;
}

void HttpServer::shutdown()
{
	::closesocket(m_socket);
	m_socket = -1;
}

void HttpServer::processHeader(Client& client)
{
	enum State
	{
		ParseRequest,
		ParseHeader
	} state = ParseRequest;

	for (const char* begin = client.m_buffer, *end = client.m_buffer + client.m_bufferSize; (begin != end) && (client.m_state == Header);)
	{
		const char* eol = begin;
		while ((eol != end) && (*eol != '\r') && (*eol != '\n')) ++eol;

		if (eol == end)
		{
			break;
		}

		switch (state)
		{
			case ParseRequest:
			{
				const char* curr = begin;

				// TODO: proper request parsing

				if (::memcmp(curr, "GET ", 4))
				{
					client.m_state = Dead;
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}
				curr += 4;
				
				if (::memcmp(curr, "/ ",2))
				{
					client.m_state = Dead;
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}
				curr += 2;

				if (!(!::memcmp(curr, "HTTP/1.0", 8) || !::memcmp(curr, "HTTP/1.1", 8)))
				{
					client.m_state = Dead;
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				state = ParseHeader;
			}
			break;

			case ParseHeader:
			{
				if (begin == eol)
				{
/*
					if (m_clientCount > 1)
					{
						client.m_state = Dead;
						sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 403 Forbidden\r\nContent-Type: text/plain\r\n\r\nOnly one streaming client allowed currently!\n");
						client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
						break;
					}
*/
					client.m_state = Streaming;
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 200 OK\r\n");
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%sContent-Type: audio/mpeg\r\n", client.m_buffer);
					if (client.m_metaData)
					{
						sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%sicy-metaint: %d\r\n", client.m_buffer, s_metaSize);
					}
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%s\r\n", client.m_buffer);

					client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
					client.m_metaOffset = s_metaSize;
					break;
				}

				if (!::memcmp(begin, "Icy-MetaData:", 13))
				{
					const char* value = begin;
					while ((eol != end) && (*eol == ' ')) ++eol;

					if (::strtoul(begin + 13, 0, 10) > 0)
					{
						client.m_metaData = true;
					}
				}
			}
			break;
		}

		while ((eol != end) && (*eol == '\r')) ++eol;
		while ((eol != end) && (*eol == '\n')) ++eol;
		begin = eol;
	}

	if ((client.m_bufferSize == sizeof(client.m_buffer)) && (client.m_state == Header))
	{
		client.m_state = Dead;
		client.m_bufferSize = client.m_bufferOffset = 0;
	}
}

void HttpServer::processStreaming(Client& client)
{
	do
	{
		if (client.m_bufferOffset != client.m_bufferSize)
		{
			break;
		}

		client.m_bufferOffset = client.m_bufferSize = 0;

		if (client.m_metaOffset > 0)
		{
			break;
		}

		client.m_metaOffset = s_metaSize;

		if (!client.m_metaData)
		{
			break;
		}

		char windowTitle[256];
		if (!GetWindowText(Notify::window(), windowTitle, sizeof(windowTitle)))
		{
			windowTitle[0] = '\0';
		}

		char buf[512];
		sprintf_s(buf, sizeof(buf), "StreamTitle='%s';", windowTitle);
		size_t length = ::strlen(buf) + 1;
		size_t fill = ((length + 15) & ~15) - length;
		size_t packetLength = length + fill;

		memset(buf + length, 0, fill);

		client.m_buffer[0] = char(packetLength / 16);
		memcpy(client.m_buffer + 1, buf, packetLength);

		client.m_bufferSize = packetLength + 1;
	}
	while (0);

	size_t maxRead = client.m_metaOffset < (sizeof(client.m_buffer) - client.m_bufferSize) ? client.m_metaOffset : sizeof(client.m_buffer) - client.m_bufferSize;
	if (!maxRead)
	{
		return;
	}

	size_t actual = m_buffer.read(client.m_buffer + client.m_bufferSize, maxRead);
	client.m_bufferSize += actual;	
	client.m_metaOffset -= actual;
}

}
