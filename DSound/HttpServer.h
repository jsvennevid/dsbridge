#ifndef dsbridge_HttpServer_h
#define dsbridge_HttpServer_h

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

#include "RingBuffer.h"

#include <windows.h>

namespace dsbridge
{

class HttpServer
{
public:
	HttpServer();
	~HttpServer();

	bool create();
	void destroy();

	short port() const;

	void write(const void* buffer, size_t count);

private:

	enum ClientState
	{
		Header,
		Close,
		Streaming,
		Cover
	};

	struct Client
	{
		Client()
		: m_socket(INVALID_SOCKET)
		, m_state(Header)
		, m_bufferSize(0)
		, m_bufferOffset(0)
		, m_metaOffset(0)
		, m_metaData(false)
		, m_cover(0)
		, m_coverOffset(0)
		, m_coverSize(0)
		, m_titleHash(0)
		, m_sendCover(false)
		{
			m_host[0] = '\0';
		}

		SOCKET m_socket;
		ClientState m_state;

		char m_buffer[8192];
		size_t m_bufferSize;
		size_t m_bufferOffset;
		size_t m_metaOffset;
		bool m_metaData;

		char* m_cover;
		size_t m_coverOffset;
		size_t m_coverSize;

		unsigned int m_titleHash;
		bool m_sendCover;

		char m_host[512];
	};

	static DWORD WINAPI threadEntry(LPVOID parameter);

	bool initialize();
	bool run();
	void shutdown();

	void processHeader(Client& client);
	void processStreaming(Client& client);
	void processCover(Client& client);

	static unsigned int hash(const char* str, size_t length);

	HANDLE m_thread;
	HANDLE m_io;
	SOCKET m_socket;

	Client* m_clients;
	DWORD m_clientCount;

	volatile bool m_running;

	RingBuffer m_buffer;
	CRITICAL_SECTION m_cs;

	time_t m_lastAnnounce;
	int m_port;

	static const unsigned int s_metaSize = 45000;
};

}

#endif
