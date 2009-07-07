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

#include "Configuration.h"

#include <stdio.h>

namespace dsbridge
{

Configuration Configuration::s_instance;
CRITICAL_SECTION Configuration::s_cs;

Configuration::Configuration()
: m_settings(0)
, m_settingsCount(0)
{
	::InitializeCriticalSection(&s_cs);

	DWORD length = ::GetModuleFileName(0, m_path, PathLength);
	m_path[length] = '\0';

	char* offset = ::strrchr(m_path, '\\');
	if (!offset)
	{
		m_path[0] = '\0';
		m_module[0] = '\0';
		return;
	}

	size_t ofslen = ::strlen(offset+1);
	::memcpy_s(m_module, sizeof(m_module), offset+1, ofslen+1);
	*(offset+1) = '\0';
	for (size_t i = 0; i < ofslen; ++i)
	{
		m_module[i] = ::toupper(m_module[i]);
	}

	sprintf_s(m_path, sizeof(m_path), "%sdsbridge.ini", m_path);

	loadSettings(m_module, m_path);
}

Configuration::~Configuration()
{
	::DeleteCriticalSection(&s_cs);
}

const char* Configuration::getString(const char* name, const char* defaultValue)
{
	const char* value;

	::EnterCriticalSection(&s_cs);
	do
	{
		value = s_instance.getStringInternal(name, defaultValue);
	}
	while (0);
	::LeaveCriticalSection(&s_cs);

	return value;
}

int Configuration::getInteger(const char* name, int defaultValue)
{
	int value;

	::EnterCriticalSection(&s_cs);
	do
	{
		value = s_instance.getIntegerInternal(name, defaultValue);
	}
	while (0);
	::LeaveCriticalSection(&s_cs);

	return value;
}

const char* Configuration::getStringInternal(const char* name, const char* defaultValue)
{
	for (unsigned int i = 0; i < m_settingsCount; ++i)
	{
		const Setting& setting = m_settings[i];

		if (::_stricmp(setting.name, name))
		{
			continue;
		}

		return setting.value;
	}

	return defaultValue;
}

int Configuration::getIntegerInternal(const char* name, int defaultValue)
{
	const char* intValue = getStringInternal(name, 0);
	if (!intValue)
	{
		return defaultValue;
	}

	int value = ::strtol(intValue, 0, 10);

	return value;
}

void Configuration::loadSettings(const char* section, const char* path)
{
	HANDLE file = INVALID_HANDLE_VALUE;
	const int bufferSize = 8192;
	char* buffer = new char[bufferSize];
	enum
	{
		SearchingForSection,
		ParsingSection
	} state = SearchingForSection;

	do
	{
		file = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		int apa2 = GetLastError();
		if (file == INVALID_HANDLE_VALUE)
		{
			break;
		}

		char* start = buffer;
		const char* begin = buffer;
		const char* end = buffer;
		for (;;)
		{
			if (begin == end)
			{
				DWORD readCount;
				if (!ReadFile(file, start, static_cast<DWORD>(bufferSize - (start - buffer)), &readCount, 0))
				{
					break;
				}

				if (!readCount)
				{
					break;
				}

				begin = start;
				end = start + readCount;
				start = buffer;
			}

			const char* eol = begin;
			while ((eol != end) && (*eol != '\r') && (*eol != '\n')) ++eol;

			if (eol == end)
			{
				// TODO: add logic to refill section
			}

			switch (state)
			{
				case SearchingForSection:
				{
					size_t len = ::strlen(section);
					if ((*begin != '[') || (((eol-1)-(begin+1)) != len) || (*(eol-1) != ']') || ::_strnicmp(begin+1, section, len))
					{
						break;
					}

					state = ParsingSection;
				}
				break;

				case ParsingSection:
				{
					const char* nameBegin = begin;
					const char* nameEnd = begin;

					while ((nameEnd != eol) && (*nameEnd != '=')) ++nameEnd;
					if (nameEnd == eol)
					{
						break;
					}

					if (*nameBegin == '[')
					{
						state = SearchingForSection;
						break;
					}

					const char* valueBegin = nameEnd+1;

					while ((valueBegin != eol) && (*valueBegin == ' ')) ++valueBegin;
					const char* valueEnd = eol;
					while ((valueEnd > (valueBegin+1)) && (*valueEnd == ' ')) --valueEnd;

					if (*valueBegin == '"')
					{
						if (*(valueEnd-1) != '"')
						{
							break;
						}

						++valueBegin;
						--valueEnd;

						if (valueBegin > valueEnd)
						{
							break;
						}
					}

					Setting temp;

					temp.name = new char[(nameEnd-nameBegin)+1];
					::memcpy(temp.name, nameBegin, (nameEnd-nameBegin));
					temp.name[(nameEnd-nameBegin)] = '\0';

					temp.value = new char[(valueEnd-valueBegin)+1];
					::memcpy(temp.value, valueBegin, (valueEnd-valueBegin));
					temp.value[(valueEnd-valueBegin)] = '\0';

					Setting* curr = new Setting[m_settingsCount+1];
					::memcpy_s(curr, sizeof(Setting) * (m_settingsCount+1), m_settings, sizeof(Setting) * m_settingsCount);
					curr[m_settingsCount] = temp;

					delete [] m_settings;
					m_settings = curr;
					++m_settingsCount;
				}
				break;
			}

			begin = eol;
			while ((begin != end) && ((*begin == '\r') || (*begin == '\n'))) ++begin;
		}
	}
	while (0);

	delete [] buffer;
	
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
	}
}

}
