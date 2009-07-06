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

#include "DirectSoundBuffer.h"
#include "Encoder.h"

#include <stdio.h>

namespace dsbridge
{

extern Encoder g_encoder;

class DirectSoundBuffer : public IDirectSoundBuffer
{
public:
	DirectSoundBuffer(LPDIRECTSOUNDBUFFER dsb, const DSBCAPS& caps);
	~DirectSoundBuffer();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSBCAPS pDSBufferCaps);
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor);
    virtual HRESULT STDMETHODCALLTYPE GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
    virtual HRESULT STDMETHODCALLTYPE GetVolume(LPLONG plVolume);
    virtual HRESULT STDMETHODCALLTYPE GetPan(LPLONG plPan);
    virtual HRESULT STDMETHODCALLTYPE GetFrequency(LPDWORD pdwFrequency);
    virtual HRESULT STDMETHODCALLTYPE GetStatus(LPDWORD pdwStatus);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc);
    virtual HRESULT STDMETHODCALLTYPE Lock(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                           LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetCurrentPosition(DWORD dwNewPosition);
    virtual HRESULT STDMETHODCALLTYPE SetFormat(LPCWAVEFORMATEX pcfxFormat);
    virtual HRESULT STDMETHODCALLTYPE SetVolume(LONG lVolume);
    virtual HRESULT STDMETHODCALLTYPE SetPan(LONG lPan);
    virtual HRESULT STDMETHODCALLTYPE SetFrequency(DWORD dwFrequency);
    virtual HRESULT STDMETHODCALLTYPE Stop();
    virtual HRESULT STDMETHODCALLTYPE Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2);
    virtual HRESULT STDMETHODCALLTYPE Restore();

private:

	struct BufferInfo
	{
		BufferInfo()
		: m_buffer(0)
		, m_size(0)
		{}

		LPVOID m_buffer;
		DWORD m_size;
	};

	LPDIRECTSOUNDBUFFER m_dsb;

	LPWAVEFORMATEX m_format;
	DWORD m_formatSize;

	BufferInfo m_internal;
	BufferInfo m_physical1;
	BufferInfo m_physical2;
};

extern Encoder g_encoder;

DirectSoundBuffer::DirectSoundBuffer(LPDIRECTSOUNDBUFFER dsb, const DSBCAPS& caps)
: m_dsb(dsb)
, m_format(0)
, m_formatSize(0)
{
	HRESULT hr;

	do
	{
		DWORD sizeWritten;
		hr = dsb->GetFormat(0, 0, &sizeWritten);
		if (FAILED(hr))
		{
			break;
		}

		char* format = new char[sizeWritten];
		hr = dsb->GetFormat(reinterpret_cast<LPWAVEFORMATEX>(format), sizeWritten, 0);
		if (FAILED(hr))
		{
			delete [] format;
			break;
		}

		m_format = reinterpret_cast<LPWAVEFORMATEX>(format);
		m_formatSize = sizeWritten;

		g_encoder.setFormat(m_format);
	}
	while (0);

	g_encoder.onCreate(caps.dwBufferBytes);
}

DirectSoundBuffer::~DirectSoundBuffer()
{
	delete [] reinterpret_cast<char*>(m_format);
	delete [] m_internal.m_buffer;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	return m_dsb->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE DirectSoundBuffer::AddRef()
{
	return m_dsb->AddRef();
}

ULONG STDMETHODCALLTYPE DirectSoundBuffer::Release()
{
	ULONG result = m_dsb->Release();
	if (!result)
	{
		delete this;
	}
	return result;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetCaps(LPDSBCAPS pDSBufferCaps)
{
	return m_dsb->GetCaps(pDSBufferCaps);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetCurrentPosition(LPDWORD pdwCurrentPlayCursor, LPDWORD pdwCurrentWriteCursor)
{
	return m_dsb->GetCurrentPosition(pdwCurrentPlayCursor, pdwCurrentWriteCursor);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetFormat(LPWAVEFORMATEX pwfxFormat, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten)
{
	return m_dsb->GetFormat(pwfxFormat, dwSizeAllocated, pdwSizeWritten);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetVolume(LPLONG plVolume)
{
	return m_dsb->GetVolume(plVolume);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetPan(LPLONG plPan)
{
	return m_dsb->GetPan(plPan);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetFrequency(LPDWORD pdwFrequency)
{
	return m_dsb->GetFrequency(pdwFrequency);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::GetStatus(LPDWORD pdwStatus)
{
	return m_dsb->GetStatus(pdwStatus);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Initialize(LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pcDSBufferDesc)
{
	return m_dsb->Initialize(pDirectSound, pcDSBufferDesc);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Lock(DWORD dwOffset, DWORD dwBytes, LPVOID *ppvAudioPtr1, LPDWORD pdwAudioBytes1,
                                       LPVOID *ppvAudioPtr2, LPDWORD pdwAudioBytes2, DWORD dwFlags)
{
	HRESULT hr;

	hr = m_dsb->Lock(dwOffset, dwBytes, ppvAudioPtr1 ? &m_physical1.m_buffer : 0, pdwAudioBytes1 ? &m_physical1.m_size : 0, ppvAudioPtr2 ? &m_physical2.m_buffer : 0, pdwAudioBytes2 ? &m_physical2.m_size : 0, dwFlags);
	if (FAILED(hr))
	{
		return hr;
	}

	DWORD bufferSize = ((m_physical1.m_size + 15) & ~15) + ((m_physical2.m_size + 15) & ~15);
	if (m_internal.m_size < bufferSize)
	{
		delete [] reinterpret_cast<char*>(m_internal.m_buffer);
		m_internal.m_buffer = new char[bufferSize];
		m_internal.m_size = bufferSize;
	}

	char* curr = static_cast<char*>(m_internal.m_buffer);

	if (ppvAudioPtr1)
	{
		(*ppvAudioPtr1) = curr;
		curr += (m_physical1.m_size + 15) & ~15;
	}

	if (pdwAudioBytes1)
	{
		(*pdwAudioBytes1) = m_physical1.m_size;
	}

	if (ppvAudioPtr2)
	{
		(*ppvAudioPtr2) = curr;
	}

	if (pdwAudioBytes2)
	{
		(*pdwAudioBytes2) = m_physical2.m_size;
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Play(DWORD dwReserved1, DWORD dwPriority, DWORD dwFlags)
{
	HRESULT result = m_dsb->Play(dwReserved1, dwPriority, dwFlags);
	if (FAILED(result))
	{
		return result;
	}

	DWORD position;
	m_dsb->GetCurrentPosition(&position, 0);
	g_encoder.onPlay(position);

	return result;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::SetCurrentPosition(DWORD dwNewPosition)
{
	return m_dsb->SetCurrentPosition(dwNewPosition);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::SetFormat(LPCWAVEFORMATEX pcfxFormat)
{
	// TODO: update cached format
	return m_dsb->SetFormat(pcfxFormat);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::SetVolume(LONG lVolume)
{
	return m_dsb->SetVolume(lVolume);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::SetPan(LONG lPan)
{
	return m_dsb->SetPan(lPan);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::SetFrequency(DWORD dwFrequency)
{
	return m_dsb->SetFrequency(dwFrequency);
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Stop()
{
	HRESULT result = m_dsb->Stop();
	if (FAILED(result))
	{
		return result;
	}

	DWORD position;
	m_dsb->GetCurrentPosition(&position, 0);
	g_encoder.onStop(position);

	return result;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Unlock(LPVOID pvAudioPtr1, DWORD dwAudioBytes1, LPVOID pvAudioPtr2, DWORD dwAudioBytes2)
{
	if (pvAudioPtr1)
	{
		::memcpy(m_physical1.m_buffer, pvAudioPtr1, dwAudioBytes1);
	}

	if (pvAudioPtr2)
	{
		::memcpy(m_physical2.m_buffer, pvAudioPtr2, dwAudioBytes2);
	}

	HRESULT hr = m_dsb->Unlock(m_physical1.m_buffer, dwAudioBytes1, m_physical2.m_buffer, dwAudioBytes2);

	if (pvAudioPtr1)
	{
		g_encoder.write(pvAudioPtr1, dwAudioBytes1);
	}

	if (pvAudioPtr2)
	{
		g_encoder.write(pvAudioPtr2, dwAudioBytes2);
	}

	m_physical1 = m_physical2 = BufferInfo();

	DWORD position;
	m_dsb->GetCurrentPosition(&position, 0);
	g_encoder.onUpdate(position);

	return hr;
}

HRESULT STDMETHODCALLTYPE DirectSoundBuffer::Restore()
{
	return m_dsb->Restore();
}

LPDIRECTSOUNDBUFFER createWrapper(LPDIRECTSOUNDBUFFER dsb)
{
	DSBCAPS caps;
	caps.dwSize = sizeof(caps);
	if (FAILED(dsb->GetCaps(&caps)))
	{
		return 0;
	}

	return reinterpret_cast<LPDIRECTSOUNDBUFFER>(new DirectSoundBuffer(dsb, caps));
}

}
