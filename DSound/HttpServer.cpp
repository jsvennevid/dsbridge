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
#include "Configuration.h"

#include <windows.h>
#include <gdiplus.h>

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
, m_port(0)
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
	saddr.sin_port = htons(m_port = Configuration::getInteger("ListenPort", 8124));
	if (::bind(m_socket, reinterpret_cast<SOCKADDR*>(&saddr), sizeof(saddr)) < 0)
	{
		Notify::update(Notify::HttpServer, Notify::Error, "Could not bind to port %d", m_port);
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

	Notify::update(Notify::HttpServer, Notify::Info, "Listening on port %d", m_port);
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
		Notify::update(Notify::HttpServer, Notify::Info, "http://localhost:%d/", m_port);
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

			case Close:
			case Streaming:
			case Cover:
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
					client.m_state = Close;
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				client.m_bufferSize += result;

				processHeader(client);
			}
			break;

			case Close:
			{
				if (!FD_ISSET(client.m_socket, &wfds))
				{
					break;
				}

				int result = ::send(client.m_socket, client.m_buffer + client.m_bufferOffset, int(client.m_bufferSize - client.m_bufferOffset), 0);
				if (result == SOCKET_ERROR)
				{
					client.m_state = Close;	
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				client.m_bufferOffset += result;
			}
			break;

			case Streaming:
			{
				if (!FD_ISSET(client.m_socket, &wfds))
				{
					break;
				}

				processStreaming(client);

				int result = ::send(client.m_socket, client.m_buffer + client.m_bufferOffset, int(client.m_bufferSize - client.m_bufferOffset), 0);
				if (result == SOCKET_ERROR)
				{
					client.m_state = Close;	
					client.m_bufferSize = client.m_bufferOffset = 0;
					break;
				}

				client.m_bufferOffset += result;
			}
			break;

			case Cover:
			{
				if (!FD_ISSET(client.m_socket, &wfds))
				{
					break;
				}

				processCover(client);

				int result = ::send(client.m_socket, client.m_buffer + client.m_bufferOffset, int(client.m_bufferSize - client.m_bufferOffset), 0);
				if (result == SOCKET_ERROR)
				{
					client.m_state = Close;	
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
		if ((client.m_state != Close) || (client.m_bufferSize != client.m_bufferOffset))
		{
			++i;
			continue;
		}

		delete [] client.m_cover;

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
	ClientState clientState = Close;

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
				const char* methodBegin = curr;
				const char* methodEnd = methodBegin;
				while ((methodEnd != eol) && (*methodEnd != ' ')) ++methodEnd;

				if ((methodEnd == eol) || ((methodEnd-methodBegin) != 3) || ::_strnicmp(methodBegin, "GET", 3))
				{
					client.m_state = Close;
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 405 Method Not Allowed\r\nAllow: GET\r\n\r\n");
					client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
					client.m_bufferOffset = 0;
					break;
				}
				curr = methodEnd + 1;

				const char* uriBegin = curr;
				const char* uriEnd = uriBegin;
				while ((uriEnd != eol) && (*uriEnd != ' ')) ++uriEnd;

				if ((uriEnd != eol) && ((uriEnd-uriBegin) == 1) && !::_strnicmp(uriBegin, "/", 1))
				{
					clientState = Streaming;
				}
				else if ((uriEnd != eol) && ((uriEnd-uriBegin) >= 6) && !::_strnicmp(uriBegin, "/cover", 6))
				{
					clientState = Cover;
				}
				else
				{
					client.m_state = Close;
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 404 Not Found\r\n\r\n");
					client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
					client.m_bufferOffset = 0;
					break;
				}
				curr = uriEnd + 1;

				if (((eol-curr) != 8) || (::_strnicmp(curr,"HTTP/1.0",8) && ::_strnicmp(curr,"HTTP/1.1",8)))
				{
					client.m_state = Close;
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 505 HTTP Version Not Supported\r\n\r\n");
					client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
					client.m_bufferOffset = 0;
					break;
				}

				state = ParseHeader;
			}
			break;

			case ParseHeader:
			{
				if (begin == eol)
				{
					client.m_state = clientState;
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "HTTP/1.0 200 OK\r\n");

					if (client.m_metaData)
					{
						sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%sicy-metaint: %d\r\n", client.m_buffer, s_metaSize);
					}

					if (client.m_state == Cover)
					{
						sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%sContent-Type: image/png\r\n", client.m_buffer);
					}
					else if (client.m_state == Streaming)
					{
					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%sContent-Type: audio/mpeg\r\n", client.m_buffer);
					}

					sprintf_s(client.m_buffer, sizeof(client.m_buffer), "%s\r\n", client.m_buffer);

					client.m_bufferSize = static_cast<int>(::strlen(client.m_buffer));
					client.m_metaOffset = s_metaSize;
					break;
				}

				const char* headerEnd = begin;
				while ((headerEnd != eol) && (*headerEnd != ':')) ++headerEnd;

				if (headerEnd == eol)
				{
					break;
				}
				const char* valueBegin = headerEnd + 1;
				while ((valueBegin != eol) && (*valueBegin == ' ')) ++valueBegin;

				if (((headerEnd - begin) == 12) && !::_strnicmp(begin, "Icy-MetaData", 12))
				{
					const char* value = begin;
					while ((eol != end) && (*eol == ' ')) ++eol;

					if (::strtoul(begin + 13, 0, 10) > 0)
					{
						client.m_metaData = true;
					}
				}

				if (((headerEnd - begin) == 4) && !::_strnicmp(begin, "Host", 4))
				{
					::memset(client.m_host, 0, sizeof(client.m_host));
					::strncpy_s(client.m_host, sizeof(client.m_host), valueBegin, eol - valueBegin);
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
		client.m_state = Close;
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

		unsigned int newHash = hash(windowTitle, ::strlen(windowTitle));
		if (newHash == client.m_titleHash && !client.m_sendCover)
		{
			client.m_buffer[0] = '\0';
			client.m_bufferSize = 1;
			break;
		}

		char* buf = client.m_buffer+1;
		size_t bufsize = sizeof(client.m_buffer)-1;
		if (newHash != client.m_titleHash)
		{
			sprintf_s(buf, bufsize, "StreamTitle='%s';", windowTitle);
			client.m_sendCover = Notify::window() && ::strlen(client.m_host) > 0;
		}
		else if (client.m_sendCover)
		{
			sprintf_s(buf, bufsize, "StreamUrl='http://%s/cover/%d.png';", client.m_host, time(0));
			client.m_sendCover = false;
		}
		size_t length = ::strlen(buf) + 1;
		size_t fill = ((length + 15) & ~15) - length;
		size_t packetLength = length + fill;

		memset(buf + length, 0, fill);

		client.m_buffer[0] = char(packetLength / 16);

		client.m_bufferSize = packetLength + 1;
		client.m_titleHash = newHash;
	}
	while (0);

	size_t maxRead = client.m_metaOffset < (sizeof(client.m_buffer) - client.m_bufferSize) ? client.m_metaOffset : sizeof(client.m_buffer) - client.m_bufferSize;
	if (!maxRead)
	{
		return;
	}

	EnterCriticalSection(&m_cs);
	do
	{
		size_t actual = m_buffer.read(client.m_buffer + client.m_bufferSize, maxRead);
		client.m_bufferSize += actual;	
		client.m_metaOffset -= actual;
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

unsigned int HttpServer::hash(const char* str, size_t length)
{
	unsigned int hash = 0;

	for(unsigned int i = 0; i < length; ++str, ++i)
	{
		unsigned int x;

		hash = (hash << 4) + (*str);

		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
		}

		hash &= ~x;
	}

	return hash;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
	return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
	return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return 0;
}

class MemoryStream : public ::IStream
{
public:
	MemoryStream()
	: m_buffer(0)
	, m_offset(0)
	, m_size(0)
	, m_capacity(0)
	{}

	~MemoryStream()
	{
		delete [] m_buffer;
	}

	// IUnknown

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
    { 
        if (iid == __uuidof(IUnknown)
            || iid == __uuidof(IStream)
            || iid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        } else
            return E_NOINTERFACE; 
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void) 
    { 
		return 1;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void) 
    {
		return 1;
    }

	// ISequentialStream

    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead)
    {
		ULONG maxRead = static_cast<ULONG>(cb > (m_size - m_offset) ? (m_size - m_offset) : cb);

		::memcpy_s(pv, cb, m_buffer, maxRead);
		m_offset += maxRead;

		if (pcbRead)
		{
			*pcbRead = maxRead;
		}

		return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten)
    {
		ULONG maxWrite = static_cast<ULONG>(cb > (m_capacity - m_offset) ? (m_capacity - m_offset) : cb);
		if (maxWrite != cb)
		{
			m_capacity = m_size + cb * 2;
			unsigned char* newBuffer = new unsigned char[static_cast<size_t>(m_capacity)];
			::memcpy(newBuffer, m_buffer, static_cast<size_t>(m_size));

			delete [] m_buffer;
			m_buffer = newBuffer;
		}

		::memcpy_s(m_buffer + m_offset, static_cast<size_t>(m_capacity - m_offset), pv, cb);
		m_offset += cb;
		m_size = m_size < m_offset ? m_offset : m_size;

		if (pcbWritten)
		{
			*pcbWritten = cb;
		}

		return S_OK;
    }

	// IStream

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER)
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
        ULARGE_INTEGER*) 
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD)                                      
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)                                       
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
    { 
        return E_NOTIMPL;   
    }
    
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **)                                  
    { 
        return E_NOTIMPL;   
    }

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
        ULARGE_INTEGER* lpNewFilePointer)
    { 
        switch(dwOrigin)
        {
			case STREAM_SEEK_SET:
			{
				if ((liDistanceToMove.QuadPart < 0) || (liDistanceToMove.QuadPart > m_size))
				{
					return STG_E_INVALIDFUNCTION;
				}

				m_offset = liDistanceToMove.QuadPart;
			}
			break;

			case STREAM_SEEK_CUR:
			{
				if (((m_offset + liDistanceToMove.QuadPart) < 0) || (m_offset + liDistanceToMove.QuadPart) > m_size)
				{
					return STG_E_INVALIDFUNCTION;
				}

				m_offset += liDistanceToMove.QuadPart;
			}
			break;

			case STREAM_SEEK_END:
			{
				if ((liDistanceToMove.QuadPart > 0) || (liDistanceToMove.QuadPart < -m_size))
				{
					return STG_E_INVALIDFUNCTION;
				}

				m_offset = m_size + liDistanceToMove.QuadPart;
			}
			break;

			default: return STG_E_INVALIDFUNCTION;
        }

		if (lpNewFilePointer)
		{
			lpNewFilePointer->QuadPart = m_offset;
		}

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
    {
        return S_OK;
    }

	unsigned char* releaseBuffer(size_t& size)
	{
		unsigned char* buffer = m_buffer;
		size = static_cast<size_t>(m_size);

		m_buffer = 0;
		m_offset = 0;
		m_size = 0;
		m_capacity = 0;

		return buffer;
	}

private:
	unsigned char* m_buffer;
	LONGLONG m_offset;
	LONGLONG m_size;
	LONGLONG m_capacity;
};

void HttpServer::processCover(Client& client)
{
	int i = 0;

	if (!client.m_cover)
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		MemoryStream stream;

		{
			RECT rect;
			HDC srcdc,destdc, windc;
			HBITMAP src, dest, oldsrc, olddest;
			HWND hwnd = Notify::window();

			::GetWindowRect(hwnd, &rect);

			srcdc = ::CreateCompatibleDC(0);
			destdc = ::CreateCompatibleDC(0);

			windc = ::GetDC(hwnd);
			{
				src = ::CreateCompatibleBitmap(windc, rect.right - rect.left, rect.bottom - rect.top);
				dest = ::CreateCompatibleBitmap(windc, 234, 234); // TODO: clip width, height
				oldsrc = (HBITMAP)::SelectObject(srcdc, src);
				olddest = (HBITMAP)::SelectObject(destdc, dest);
				::PrintWindow(hwnd, srcdc, 0);
			}
			::ReleaseDC(hwnd, windc);

			::BitBlt(destdc, 0, 0, 234, 234, srcdc, 0, rect.bottom - 275, SRCCOPY);

			Gdiplus::Bitmap bitmap(dest, NULL);
			CLSID clsid;
			GetEncoderClsid(L"image/png", &clsid);
			bitmap.Save(&stream, &clsid);

			::SelectObject(srcdc, oldsrc);
			::SelectObject(destdc, olddest);

			DeleteObject(srcdc);
			DeleteObject(destdc);

			DeleteObject(src);
			DeleteObject(dest);
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);

		client.m_cover = reinterpret_cast<char*>(stream.releaseBuffer(client.m_coverSize));
		if (!client.m_cover || !client.m_coverSize)
		{
			client.m_state = Close;
		}
	}

	do
	{
		if (client.m_bufferOffset != client.m_bufferSize)
		{
			break;
		}

		if (client.m_coverOffset == client.m_coverSize)
		{
			client.m_state = Close;
			break;
		}

		size_t maxCopy = (client.m_coverSize - client.m_coverOffset) > sizeof(client.m_buffer) ? sizeof(client.m_buffer) : (client.m_coverSize - client.m_coverOffset);

		::memcpy_s(client.m_buffer, sizeof(client.m_buffer), client.m_cover + client.m_coverOffset, maxCopy);

		client.m_bufferOffset = 0;
		client.m_bufferSize = maxCopy;
		client.m_coverOffset += maxCopy;
	}
	while (0);
}

}
