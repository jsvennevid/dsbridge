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

#include "Notify.h"

#include "resource.h"

#include <stdio.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShellAPI.h>

namespace dsbridge
{

Notify Notify::s_instance;
HINSTANCE Notify::s_dll;

Notify::Notify()
: m_iconAdded(false)
, m_isConnected(false)
, m_lastTime(0)
, m_lastState(Info)
, m_window(0)
{
	InitializeCriticalSection(&m_cs);
	m_window = findMainWindow();
}

Notify::~Notify()
{
	DeleteCriticalSection(&m_cs);
}

void Notify::update(ClassType classType, State state, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	s_instance.updateInternal(classType, state, format, ap);
	va_end(ap);
}

void Notify::setDllInstance(HINSTANCE hInstDLL)
{
	s_dll = hInstDLL;
}

void Notify::setConnected(bool connected)
{
	s_instance.setConnectedInternal(connected);
}

struct WindowInformation
{
	DWORD m_pid;
	HWND m_result;
};

HWND Notify::findMainWindow()
{
	WindowInformation info;
	info.m_pid = GetCurrentProcessId();
	info.m_result = 0;
	EnumWindows(windowEnumerator, (LPARAM)&info);
	return info.m_result;
}

BOOL CALLBACK Notify::windowEnumerator(HWND hwnd, LPARAM lParam)
{
	WindowInformation* info = reinterpret_cast<WindowInformation*>(lParam);
	DWORD processId;

	char windowClass[256];
	RealGetWindowClass(hwnd, windowClass, sizeof(windowClass));

	char windowName[256];
	if (!GetWindowText(hwnd, windowName, sizeof(windowName)))
	{
		return TRUE;
	}

	GetWindowThreadProcessId(hwnd, &processId);

	if (processId == info->m_pid)
	{
		info->m_result = hwnd;
		return FALSE;
	}

	return TRUE;
}

void Notify::updateInternal(ClassType classType, State state, const char* format, va_list ap)
{
	NOTIFYICONDATA data;
	bool iconAdded = m_iconAdded;
	bool noNewStatus = false;

	EnterCriticalSection(&m_cs);
	do
	{
		time_t newTime = time(0);
		switch (m_lastState)
		{
			case Info:
			{
			}
			break;

			case Warning:
			{
				if (((newTime - m_lastTime) < 5) && state == Info)
					noNewStatus = true;
			}
			break;

			case Error:
			{
				noNewStatus = true;
			}
			break;
		}

		if (noNewStatus)
		{
			break;
		}

		m_lastTime = newTime;
		m_lastState = state;

		char buf[64];
		vsnprintf_s(buf+2, sizeof(buf)-2, sizeof(buf)-2, format, ap);
		switch (classType)
		{
			case DirectSound: buf[0] = 'D'; break;
			case Encoder: buf[0] = 'E'; break;
			case HttpServer: buf[0] = 'H'; break;
		}
		buf[1] = ':';

		memset(&data, 0, sizeof(data));
		data.cbSize = sizeof(data);
		data.uFlags = NIF_ICON | NIF_TIP;
		data.hWnd = m_window;
		data.uID = 1337;
		data.hIcon = getIcon(state);
		strcpy_s(data.szTip, sizeof(data.szTip), buf);

		m_iconAdded = true;

#if defined(_DEBUG)
		OutputDebugString(buf);
		OutputDebugString("\n");
#endif
	}
	while (0);
	LeaveCriticalSection(&m_cs);

	if (!noNewStatus)
	{
		Shell_NotifyIcon(iconAdded ? NIM_MODIFY : NIM_ADD, &data);
	}
}

void Notify::setConnectedInternal(bool connected)
{
	EnterCriticalSection(&m_cs);
	do
	{
		m_isConnected = connected;
	}
	while (0);
	LeaveCriticalSection(&m_cs);
}

HICON Notify::getIcon(State state)
{
	LPCSTR resource;
	switch (state)
	{
		case Info: resource = MAKEINTRESOURCE(m_isConnected ? IDI_ICONCONNECTED : IDI_ICONIDLE); break;
		case Warning:  resource = MAKEINTRESOURCE(IDI_ICONWARNING); break;
		case Error: resource = MAKEINTRESOURCE(IDI_ICONERROR); break;
	}
	return LoadIcon(s_dll, resource);
}

}
