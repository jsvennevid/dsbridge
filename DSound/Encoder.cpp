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

#include "Encoder.h"
#include "HttpServer.h"
#include "Notify.h"
#include "ExceptionHandler.h"
#include "Configuration.h"

#include <stdio.h>

namespace dsbridge
{

extern HttpServer g_httpServer;

Encoder::Encoder()
: m_playing(false)
, m_destroyed(false)
, m_raw(0)
, m_current(0)
, m_goal(0)
, m_bufferSize(0)
{
	InitializeCriticalSection(&m_cs);
}

Encoder::~Encoder()
{
}

bool Encoder::create()
{
	if (!m_buffer.create(1024 * 1024))
	{
		Notify::update(Notify::Encoder, Notify::Error, "Could not create ringbuffer");
		return false;
	}

	m_module = LoadLibrary("lame_enc.dll");
	if (!m_module)
	{
		Notify::update(Notify::Encoder, Notify::Error, "Could not load lame_enc.dll");
		return false;
	}

	BEINITSTREAM beInitStream = reinterpret_cast<BEINITSTREAM>(GetProcAddress(m_module, "beInitStream"));
	if (!beInitStream)
	{
		Notify::update(Notify::Encoder, Notify::Error, "Could not resolve beInitStream()");
		return false;
	}

	BE_CONFIG init;
	memset(&init, 0, sizeof(init));
	init.dwConfig = BE_CONFIG_LAME;
	init.format.LHV1.dwStructVersion = 1;
	init.format.LHV1.dwStructSize = sizeof(init);
	init.format.LHV1.dwSampleRate = 44100;
	init.format.LHV1.dwReSampleRate = 0;
	init.format.LHV1.nMode = BE_MP3_MODE_JSTEREO;
	init.format.LHV1.dwBitrate = init.format.LHV1.dwMaxBitrate = Configuration::getInteger("MP3BitRate", 192);
	init.format.LHV1.nPreset = LQP_NOPRESET;
	init.format.LHV1.bCopyright = false;
	init.format.LHV1.bCRC = false;
	init.format.LHV1.bOriginal = false;
	init.format.LHV1.bPrivate = false;
	init.format.LHV1.bWriteVBRHeader = false;
	init.format.LHV1.bEnableVBR = false;
	init.format.LHV1.nVBRQuality = 0;
	init.format.LHV1.dwVbrAbr_bps = 0;
	init.format.LHV1.bNoRes = false;
	BE_ERR result = beInitStream(&init, &m_samples, &m_outputSize, &m_stream);
	if (result != BE_ERR_SUCCESSFUL)
	{
		Notify::update(Notify::Encoder, Notify::Error, "beInitStream() failed - %08x", result);
		return false;
	}

	m_outputBuffer = new BYTE[m_outputSize];
	m_inputBuffer = new BYTE[m_samples * 2 * 2];

	m_thread = CreateThread(0, 0, threadEntry, this, CREATE_SUSPENDED, 0);
	if(!m_thread)
	{
		Notify::update(Notify::Encoder, Notify::Error, "Could not create thread");
		return false;
	}

	ResumeThread(m_thread);
	return true;
}

void Encoder::setFormat(LPWAVEFORMATEX format)
{
	EnterCriticalSection(&m_cs);

	do
	{
		// TODO: configure resampling / channel mixing
/*
		char buf[1024];
		sprintf_s(buf, sizeof(buf),
		"wFormatTag: %d\n"
		"nChannels: %d\n"
		"nSamplesPerSec: %d\n"
		"nAvgBytesPerSec: %d\n"
		"nBlockAlign: %d\n"
		"wBitsPerSample: %d\n"
		"cbSize: %d\n",
		format->wFormatTag,
		format->nChannels,
		format->nSamplesPerSec,
		format->nAvgBytesPerSec,
		format->nBlockAlign,
		format->wBitsPerSample,
		format->cbSize);

		MessageBox(0, buf, "DirectSound Bridge", MB_ICONERROR|MB_OK);
*/
	}
	while (0);

	LeaveCriticalSection(&m_cs);
}

void Encoder::write(const void* buffer, size_t count)
{
	EnterCriticalSection(&m_cs);

	do
	{
		m_buffer.write(buffer, count);
	}
	while (0);

	LeaveCriticalSection(&m_cs);
}

void Encoder::onCreate(DWORD bufferSize)
{
	EnterCriticalSection(&m_cs);
	do
	{
		m_playing = false;
		m_destroyed = false;
		m_raw = 0;
		m_current = 0;
		m_goal = 0;
		m_bufferSize = bufferSize;
		m_lastTimer.QuadPart = 0;
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

void Encoder::onPlay(DWORD position)
{
	EnterCriticalSection(&m_cs);
	do
	{
		if (!m_playing)
		{
			QueryPerformanceCounter(&m_lastTimer);
			m_playing = true;
		}

		updatePosition(position);
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

void Encoder::onStop(DWORD position)
{
	EnterCriticalSection(&m_cs);
	do
	{
		if (m_playing)
		{
			updatePosition(position);
		}
		m_playing = false;
	}
	while (0);
	LeaveCriticalSection(&m_cs);

	onDestroy();
}

void Encoder::onUpdate(DWORD position)
{
	EnterCriticalSection(&m_cs);
	do
	{
		if (m_playing)
		{
			updatePosition(position);
		}
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

void Encoder::onDestroy()
{
	EnterCriticalSection(&m_cs);
	do
	{
		m_destroyed = true;

		// TODO: use a waitable object instead

		while (m_buffer.written() > (m_goal - m_current))
		{
			LeaveCriticalSection(&m_cs);
			SleepEx(10, TRUE);
			EnterCriticalSection(&m_cs);
		}

		m_raw = 0;
		m_goal -= m_current;
		m_current = 0;

		m_playing = false;
		m_destroyed = false;
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

DWORD WINAPI Encoder::threadEntry(LPVOID parameter)
{
	__try
	{
		Notify::update(Notify::Encoder, Notify::Info, "Started");
		Encoder* encoder = static_cast<Encoder*>(parameter);

		while (encoder->run())
		{
			SleepEx(1, TRUE);
		}

		Notify::update(Notify::Encoder, Notify::Info, "Stopped");
	}
	__except(ExceptionHandler::filter("Encoder", GetExceptionInformation()))
	{
	}
	return 0;
}

bool Encoder::run()
{
	for (;;)
	{
		bool done = false;
		EnterCriticalSection(&m_cs);
		do
		{
			size_t readMax = (m_goal - m_current);
			size_t readNeeded = m_samples * 2;
			size_t readAvailable = m_buffer.written();

			readAvailable = readMax < readAvailable ? readMax : readAvailable;

			if (m_destroyed)
			{
				if (readAvailable < readNeeded)
				{
					m_buffer.discard(m_buffer.written() - readMax);
					done = true;
					break;
				}
			}


			if (readAvailable < readNeeded)
			{
				done = true;
				break;
			}

			m_buffer.read(m_inputBuffer, readNeeded);

			m_current += readNeeded;
		}
		while (0);
		LeaveCriticalSection(&m_cs);

		if (done)
		{
			break;
		}

		DWORD bytesWritten;
		static BEENCODECHUNK beEncodeChunk = reinterpret_cast<BEENCODECHUNK>(GetProcAddress(m_module, "beEncodeChunk"));
		if (!beEncodeChunk)
		{
			Notify::update(Notify::Encoder, Notify::Error, "Could not resolve beEncodeChunk()");
			return false;
		}
		BE_ERR result = beEncodeChunk(m_stream, m_samples, (PSHORT)m_inputBuffer, m_outputBuffer, &bytesWritten);
		if (result != BE_ERR_SUCCESSFUL)
		{
			Notify::update(Notify::Encoder, Notify::Warning, "beEncodeChunk() failed - %08x", result);
			break;
		}

		g_httpServer.write(m_outputBuffer, bytesWritten);
	}

	return true;
}

void Encoder::updatePosition(DWORD position)
{
	static int useOldEncodingLimiter = Configuration::getInteger("UseOldEncodingLimiter");
	if (useOldEncodingLimiter)
	{
		DWORD actual = position;
		if (position < m_raw)
		{
			actual += m_bufferSize;
		}

		m_goal += actual - m_raw;
	}
	else
	{
		LARGE_INTEGER currTimer;
		QueryPerformanceCounter(&currTimer);

		if (m_playing)
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);

			double delta = (currTimer.QuadPart - m_lastTimer.QuadPart) / static_cast<double>(freq.QuadPart);
			DWORD spent = static_cast<DWORD>(delta * 44100);

			m_goal += spent * (2*2);
		}

		m_lastTimer = currTimer;
	}

	m_raw = position;
}

}