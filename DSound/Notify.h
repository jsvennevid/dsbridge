#ifndef dsbridge_Notify_h
#define dsbridge_Notify_h

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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdarg.h>

namespace dsbridge
{

class Notify
{
public:
	enum ClassType
	{
		DirectSound,
		Encoder,
		HttpServer
	};

	enum State
	{
		Info,
		Warning,
		Error
	};

	Notify();
	~Notify();

	static void update(ClassType classType, State state, const char* format, ...);
	static void setDllInstance(HINSTANCE hInstDLL);
	static void setConnected(bool connected);
	static HWND window() { return s_instance.m_window; }

private:

	static BOOL CALLBACK windowEnumerator(HWND hwnd, LPARAM lParam);
	void updateInternal(ClassType classType, State state, const char* format, va_list ap);
	void setConnectedInternal(bool connected);

	HICON getIcon(State state);

	HWND findMainWindow();

	static Notify s_instance;
	static HINSTANCE s_dll;

	bool m_iconAdded;
	bool m_isConnected;
	HWND m_window;
	CRITICAL_SECTION m_cs;

	time_t m_lastTime;
	State m_lastState;
};

}

#endif
