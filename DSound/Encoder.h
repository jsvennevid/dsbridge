#ifndef dsbridge_Encoder_h
#define dsbridge_Encoder_h

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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <audiodefs.h>

#include "BladeMP3EncDLL.h"

namespace dsbridge
{

class Encoder
{
public:
	Encoder();
	~Encoder();

	bool create();
	void destroy();

	void setFormat(LPWAVEFORMATEX format);
	void write(const void* buffer, size_t count);

private:

	static DWORD WINAPI threadEntry(LPVOID parameter);

	bool run();

	HMODULE m_module;
	WAVEFORMATEX m_format;
	CRITICAL_SECTION m_cs;

	HANDLE m_thread;

	HBE_STREAM m_stream;
	DWORD m_samples;

	DWORD m_outputSize;
	PBYTE m_outputBuffer;
	PBYTE m_inputBuffer;

	RingBuffer m_buffer;
};

}

#endif
