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

#include "DirectSound.h"
#include "DirectSoundBuffer.h"

namespace dsbridge
{

class DirectSound : public IDirectSound
{
public:
	DirectSound(LPDIRECTSOUND ds);

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

    virtual HRESULT STDMETHODCALLTYPE CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter);
    virtual HRESULT STDMETHODCALLTYPE GetCaps(LPDSCAPS pDSCaps);
    virtual HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate);
    virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND hwnd, DWORD dwLevel);
    virtual HRESULT STDMETHODCALLTYPE Compact();
    virtual HRESULT STDMETHODCALLTYPE GetSpeakerConfig(LPDWORD pdwSpeakerConfig);
    virtual HRESULT STDMETHODCALLTYPE SetSpeakerConfig(DWORD dwSpeakerConfig);
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCGUID pcGuidDevice);

private:

	LPDIRECTSOUND m_ds;
};

DirectSound::DirectSound(LPDIRECTSOUND ds)
: m_ds(ds)
{}

HRESULT STDMETHODCALLTYPE DirectSound::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	return m_ds->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE DirectSound::AddRef()
{
	return m_ds->AddRef();
}

ULONG STDMETHODCALLTYPE DirectSound::Release()
{
	ULONG result = m_ds->Release();
	if (!result)
	{
		delete this;
	}
	return result;
}

HRESULT STDMETHODCALLTYPE DirectSound::CreateSoundBuffer(LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter)
{
	LPDIRECTSOUNDBUFFER localBuffer;
	HRESULT hr = m_ds->CreateSoundBuffer(pcDSBufferDesc, &localBuffer, pUnkOuter);
	if (FAILED(hr))
	{
		return hr;
	}

	(*ppDSBuffer) = createWrapper(localBuffer);
	if (!*ppDSBuffer)
	{
		localBuffer->Release();
		return DSERR_OUTOFMEMORY;
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE DirectSound::GetCaps(LPDSCAPS pDSCaps)
{
	return m_ds->GetCaps(pDSCaps);
}

HRESULT STDMETHODCALLTYPE DirectSound::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate)
{
	return m_ds->DuplicateSoundBuffer(pDSBufferOriginal, ppDSBufferDuplicate);
}

HRESULT STDMETHODCALLTYPE DirectSound::SetCooperativeLevel(HWND hwnd, DWORD dwLevel)
{
	return m_ds->SetCooperativeLevel(hwnd, dwLevel);
}

HRESULT STDMETHODCALLTYPE DirectSound::Compact()
{
	return m_ds->Compact();
}

HRESULT STDMETHODCALLTYPE DirectSound::GetSpeakerConfig(LPDWORD pdwSpeakerConfig)
{
	return m_ds->GetSpeakerConfig(pdwSpeakerConfig);
}

HRESULT STDMETHODCALLTYPE DirectSound::SetSpeakerConfig(DWORD dwSpeakerConfig)
{
	return m_ds->SetSpeakerConfig(dwSpeakerConfig);
}

HRESULT STDMETHODCALLTYPE DirectSound::Initialize(LPCGUID pcGuidDevice)
{
	return m_ds->Initialize(pcGuidDevice);
}

LPDIRECTSOUND createWrapper(LPDIRECTSOUND pDS)
{
	return reinterpret_cast<LPDIRECTSOUND>(new DirectSound(pDS));
}

}