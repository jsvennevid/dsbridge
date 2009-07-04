#ifndef dsbridge_ExceptionHandler_h
#define dsbridge_ExceptionHandler_h

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
#include <dbghelp.h>

namespace dsbridge
{

class ExceptionHandler
{
public:

	ExceptionHandler();
	~ExceptionHandler();

	static LONG WINAPI filter(const char* name, struct _EXCEPTION_POINTERS* info);

	bool createDump(DWORD thread, const char* name, struct _EXCEPTION_POINTERS* info);

private:

	typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(IN HANDLE hProcess, IN DWORD ProcessId, IN HANDLE hFile, IN MINIDUMP_TYPE DumpType, IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL);

	bool initialize();

	HMODULE m_module;

	MINIDUMPWRITEDUMP MiniDumpWriteDump;
};


}

#endif
