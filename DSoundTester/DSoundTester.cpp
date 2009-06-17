#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <audiodefs.h>
#include <dsound.h>
#include <winsock2.h>

typedef HRESULT (WINAPI *DSCreate)(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HMODULE module = LoadLibrary("dsound.dll");
	if (!module)
	{
		OutputDebugString("Could not load DirectSound");
		return 0;
	}

	WSADATA data;
	WSAStartup(MAKEWORD(2,0), &data);

	DSCreate func = reinterpret_cast<DSCreate>(GetProcAddress(module, "DirectSoundCreate"));
	if (!func)
	{
		OutputDebugString("Could not find DirectSoundCreate");
		return 0;
	}

	LPDIRECTSOUND ds;
	if (FAILED(func(0, &ds, 0)))
	{
		OutputDebugString("Could not construct IDirectSound instance");
		return 0;
	}

	while (1)
	{
		SleepEx(1, TRUE);
	}

	return 0;
}