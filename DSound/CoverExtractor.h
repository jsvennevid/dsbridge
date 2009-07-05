#ifndef dsbridge_CoverExtractor_h
#define dsbridge_CoverExtractor_h

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

class CoverExtractor
{
public:

	struct Cover
	{
		Cover()
		: image(0)
		, length(0)
		{}

		~Cover()
		{
			delete [] image;
		}

		char* image;
		size_t length;
	};

	static Cover* getCoverImage(HWND hWnd);

private:

	struct WindowState
	{
		bool iconic;

		RECT position;
		ANIMATIONINFO animation;

		COLORREF key;
		BYTE alpha;
		DWORD flags;

		LONG windowStyle;
		LONG windowExStyle;
	};

	class MemoryStream : public ::IStream
	{
	public:
		MemoryStream()
		: m_buffer(0)
		, m_offset(0)
		, m_size(0)
		, m_capacity(0)
		{}

		virtual ~MemoryStream();

		// IUnknown

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
		virtual ULONG STDMETHODCALLTYPE AddRef(void);
		virtual ULONG STDMETHODCALLTYPE Release(void);

		// ISequentialStream

		virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
		virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);

		// IStream

		virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER);
		virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
			ULARGE_INTEGER*);
		virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);
		virtual HRESULT STDMETHODCALLTYPE Revert(void);
		virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
		virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
		virtual HRESULT STDMETHODCALLTYPE Clone(IStream **);
		virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
			ULARGE_INTEGER* lpNewFilePointer);
		virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);

	private:
		unsigned char* m_buffer;
		LONGLONG m_offset;
		LONGLONG m_size;
		LONGLONG m_capacity;
	};

	static WindowState showWindow(HWND hWnd);
	static void restoreWindow(HWND hWnd, const WindowState& state);

	static int getEncoderClsid(const WCHAR* format, CLSID* pClsid);
	static Cover* createCoverFromStream(::IStream* stream);
};

}

#endif
