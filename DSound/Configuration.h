#ifndef dsbridge_Configuration_h
#define dsbridge_Configuration_h

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

namespace dsbridge
{

class Configuration
{
public:

	static const char* getString(const char* name, const char* defaultValue = "");
	static int getInteger(const char* name, int defaultValue = 0);

private:

	enum
	{
		PathLength = MAX_PATH
	};

	struct Setting
	{
		char* name;
		char* value;
	};

	Configuration();
	~Configuration();

	const char* getStringInternal(const char* name, const char* defaultValue = "");
	int getIntegerInternal(const char* name, int defaultValue = 0);
	void loadSettings(const char* section, const char* path);

	char m_module[PathLength + 1];
	char m_path[PathLength+1];

	Setting* m_settings;
	size_t m_settingsCount;

	static Configuration s_instance;
	static CRITICAL_SECTION s_cs;
};

}

#endif
