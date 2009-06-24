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

#include "DSBridge.h"
#include "DirectSound.h"
#include "Configuration.h"
#include "HttpServer.h"
#include "Encoder.h"
#include "Notify.h"

#include <stdio.h>

namespace dsbridge
{

#define DSBRIDGE_BEGINCALL(func)
#define DSBRIDGE_ENDCALL(func)
#define DSBRIDGE_ASSERT(expr,desc)

typedef HRESULT (WINAPI *DSCreate)(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);

typedef HRESULT (WINAPI *DSEnumerateA)(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
typedef HRESULT (WINAPI *DSEnumerateW)(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);

typedef HRESULT (WINAPI *DSDllCanUnloadNow)();
typedef HRESULT (WINAPI *DSDllGetClassObject)(__in REFCLSID rclsid, __in REFIID riid, __out LPVOID *ppv);

typedef HRESULT (WINAPI *DSCaptureCreate)(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE *ppDSC, LPUNKNOWN pUnkOuter);
typedef HRESULT (WINAPI *DSCaptureEnumerateA)(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);
typedef HRESULT (WINAPI *DSCaptureEnumerateW)(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext);

typedef HRESULT (WINAPI *DSGetDeviceID)(LPCGUID pGuidSrc, LPGUID pGuidDest);

typedef HRESULT (WINAPI *DSFullDuplexCreate)(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice,
        LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc, HWND hWnd,
        DWORD dwLevel, LPDIRECTSOUNDFULLDUPLEX* ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
        LPDIRECTSOUNDBUFFER8 *ppDSBuffer8, LPUNKNOWN pUnkOuter);
typedef HRESULT (WINAPI *DSCreate8)(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter);
typedef HRESULT (WINAPI *DSCaptureCreate8)(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE8 *ppDSC8, LPUNKNOWN pUnkOuter);

HttpServer g_httpServer;
Encoder g_encoder;
Configuration g_configuration;

static LPVOID getDSProc(const char* name)
{
	static HMODULE module = 0;
	if (!module)
	{
		static char location[MAX_PATH];
		int length = GetSystemDirectory(location, sizeof(location));
		if (!length || (length >= sizeof(location)))
		{
			Notify::update(Notify::DirectSound, Notify::Error, "Could not locate system directory", location);
			return 0;
		}

		sprintf_s(location, sizeof(location), "%s\\DSOUND.DLL", location);
		module = LoadLibrary(location);
		DSBRIDGE_ASSERT(module, "Could not load DSOUND.DLL");
		if (!module)
		{
			Notify::update(Notify::DirectSound, Notify::Error, "Could not load %s", location);
			return 0;
		}

		if (!g_httpServer.create())
		{
			return 0;
		}

		if (!g_encoder.create())
		{
			return 0;
		}

		Notify::update(Notify::DirectSound, Notify::Info, "Loaded");
	}

	return GetProcAddress(module, name);
}

HRESULT WINAPI DSBDirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCreate func = static_cast<DSCreate>(getDSProc("DirectSoundCreate"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCreate");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		LPDIRECTSOUND pLocalDS;
		hr = func(pcGuidDevice, &pLocalDS, pUnkOuter);
		if (FAILED(hr))
		{
			break;
		}

		*(ppDS) = createWrapper(pLocalDS);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSEnumerateA func = static_cast<DSEnumerateA>(getDSProc("DirectSoundEnumerateA"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundEnumerateA");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pDSEnumCallback, pContext);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSEnumerateW func = static_cast<DSEnumerateW>(getDSProc("DirectSoundEnumerateW"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundEnumerateW");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pDSEnumCallback, pContext);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDllCanUnloadNow()
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSDllCanUnloadNow func = static_cast<DSDllCanUnloadNow>(getDSProc("DllCanUnloadNow"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DllCanUnloadNow");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func();
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, __out LPVOID *ppv)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSDllGetClassObject func = static_cast<DSDllGetClassObject>(getDSProc("DllGetClassObject"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DllGetClassObject");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(rclsid, riid, ppv);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundCaptureCreate(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE *ppDSC, LPUNKNOWN pUnkOuter)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCaptureCreate func = static_cast<DSCaptureCreate>(getDSProc("DirectSoundCaptureCreate"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCaptureCreate");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pcGuidDevice, ppDSC, pUnkOuter);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCaptureEnumerateA func = static_cast<DSCaptureEnumerateA>(getDSProc("DirectSoundCaptureEnumerateA"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCaptureEnumerateA");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pDSEnumCallback, pContext);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pDSEnumCallback, LPVOID pContext)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCaptureEnumerateW func = static_cast<DSCaptureEnumerateW>(getDSProc("DirectSoundCaptureEnumerateW"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCaptureEnumerateW");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pDSEnumCallback, pContext);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}


HRESULT WINAPI DSBGetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSGetDeviceID func = static_cast<DSGetDeviceID>(getDSProc("GetDeviceID"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "GetDeviceID");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pGuidSrc, pGuidDest);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundFullDuplexCreate(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice,
        LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc, HWND hWnd,
        DWORD dwLevel, LPDIRECTSOUNDFULLDUPLEX* ppDSFD, LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
        LPDIRECTSOUNDBUFFER8 *ppDSBuffer8, LPUNKNOWN pUnkOuter)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSFullDuplexCreate func = static_cast<DSFullDuplexCreate>(getDSProc("DirectSoundFullDuplexCreate"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundFullDuplexCreate");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pcGuidCaptureDevice, pcGuidRenderDevice, pcDSCBufferDesc, pcDSBufferDesc, hWnd, dwLevel, ppDSFD, ppDSCBuffer8, ppDSBuffer8, pUnkOuter);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS8, LPUNKNOWN pUnkOuter)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCreate8 func = static_cast<DSCreate8>(getDSProc("DirectSoundCreate8"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCreate8");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pcGuidDevice, ppDS8, pUnkOuter);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

HRESULT WINAPI DSBDirectSoundCaptureCreate8(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE8 *ppDSC8, LPUNKNOWN pUnkOuter)
{
	HRESULT hr;
	DSBRIDGE_BEGINCALL(__FUNCTION__);
	do
	{
		DSCaptureCreate8 func = static_cast<DSCaptureCreate8>(getDSProc("DirectSoundCaptureCreate8"));
		DSBRIDGE_ASSERT(func, "Could not resolve " "DirectSoundCaptureCreate8");
		if (!func)
		{
			hr = DSERR_UNSUPPORTED;
			break;
		}

		hr = func(pcGuidDevice, ppDSC8, pUnkOuter);
	}
	while (0);
	DSBRIDGE_ENDCALL(__FUNCTION__);
	return hr;
}

}
