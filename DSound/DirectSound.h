#ifndef dsbridge_DirectSound_h
#define dsbridge_DirectSound_h

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

#include <windows.h>
#include <audiodefs.h>
#include <dsound.h>

namespace dsbridge
{

#undef INTERFACE
#define INTERFACE IDirectSound

DECLARE_INTERFACE_(IDirectSound, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD(CreateSoundBuffer)    (THIS_ LPCDSBUFFERDESC pcDSBufferDesc, LPDIRECTSOUNDBUFFER *ppDSBuffer, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDSCAPS pDSCaps) PURE;
    STDMETHOD(DuplicateSoundBuffer) (THIS_ LPDIRECTSOUNDBUFFER pDSBufferOriginal, LPDIRECTSOUNDBUFFER *ppDSBufferDuplicate) PURE;
    STDMETHOD(SetCooperativeLevel)  (THIS_ HWND hwnd, DWORD dwLevel) PURE;
    STDMETHOD(Compact)              (THIS) PURE;
    STDMETHOD(GetSpeakerConfig)     (THIS_ LPDWORD pdwSpeakerConfig) PURE;
    STDMETHOD(SetSpeakerConfig)     (THIS_ DWORD dwSpeakerConfig) PURE;
    STDMETHOD(Initialize)           (THIS_ LPCGUID pcGuidDevice) PURE;
};

LPDIRECTSOUND createWrapper(LPDIRECTSOUND);

}

#endif
