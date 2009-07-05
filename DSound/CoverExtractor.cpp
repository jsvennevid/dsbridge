#include "CoverExtractor.h"

#include <gdiplus.h>

namespace dsbridge
{

CoverExtractor::Cover* CoverExtractor::getCoverImage(HWND hWnd)
{
	if (!hWnd || hWnd == GetDesktopWindow())
	{
		return 0;
	}

	MemoryStream stream;
	WindowState oldState = showWindow(hWnd);
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		{
			RECT rect;
			HDC srcdc,destdc, windc;
			HBITMAP src, dest, oldsrc, olddest;

			::GetWindowRect(hWnd, &rect);

			srcdc = ::CreateCompatibleDC(0);
			destdc = ::CreateCompatibleDC(0);

			windc = ::GetDC(hWnd);
			{
				src = ::CreateCompatibleBitmap(windc, rect.right - rect.left, rect.bottom - rect.top);
				dest = ::CreateCompatibleBitmap(windc, 234, 234); // TODO: clip width, height
				oldsrc = (HBITMAP)::SelectObject(srcdc, src);
				olddest = (HBITMAP)::SelectObject(destdc, dest);
				::PrintWindow(hWnd, srcdc, 0);
			}
			::ReleaseDC(hWnd, windc);

			::BitBlt(destdc, 0, 0, 234, 234, srcdc, 0, (rect.bottom-rect.top) - 275, SRCCOPY);

			Gdiplus::Bitmap bitmap(dest, NULL);
			CLSID clsid;
			getEncoderClsid(L"image/png", &clsid);
			bitmap.Save(&stream, &clsid);

			::SelectObject(srcdc, oldsrc);
			::SelectObject(destdc, olddest);

			DeleteObject(srcdc);
			DeleteObject(destdc);

			DeleteObject(src);
			DeleteObject(dest);
		}

		Gdiplus::GdiplusShutdown(gdiplusToken);
	}

	restoreWindow(hWnd, oldState);

	return createCoverFromStream(&stream);
}

CoverExtractor::WindowState CoverExtractor::showWindow(HWND hWnd)
{
	WindowState state;
	state.animation.cbSize = sizeof(state.animation);

	state.iconic = !!IsIconic(hWnd);
	state.windowStyle = GetWindowLong(hWnd, GWL_STYLE);

	if (!state.iconic && (state.windowStyle & WS_VISIBLE))
	{
		return state;
	}

	::SystemParametersInfo(SPI_GETANIMATION, sizeof(state.animation), &state.animation, 0);
	::GetLayeredWindowAttributes(hWnd, &state.key, &state.alpha, &state.flags);
	state.windowExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

	ANIMATIONINFO newAnimation;
	newAnimation.cbSize = sizeof(newAnimation);
	newAnimation.iMinAnimate = 0;

	::SystemParametersInfo(SPI_SETANIMATION, sizeof(newAnimation), &newAnimation, 0);
	::SetWindowLong(hWnd, GWL_EXSTYLE, state.windowExStyle | WS_EX_LAYERED);
	::SetLayeredWindowAttributes(hWnd, 0, 1, LWA_ALPHA);

	if (!(state.windowStyle & WS_VISIBLE))
	{
		::SetWindowLong(hWnd, GWL_EXSTYLE, state.windowExStyle | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
		::GetWindowRect(hWnd, &state.position);
		::SetWindowPos(hWnd, 0, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOOWNERZORDER);
	}

	if (state.iconic)
	{
		::ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	}

	return state;
}

void CoverExtractor::restoreWindow(HWND hWnd, const WindowState& state)
{
	if (!state.iconic && (state.windowStyle & WS_VISIBLE))
	{
		return;
	}

	if (state.iconic)
	{
		ShowWindow(hWnd, SW_MINIMIZE);
	}

	if (!(state.windowStyle & WS_VISIBLE))
	{
		::SetWindowPos(hWnd, 0, state.position.left, state.position.top, state.position.right-state.position.left, state.position.bottom-state.position.top, SWP_NOACTIVATE|SWP_NOZORDER|SWP_HIDEWINDOW|SWP_NOOWNERZORDER);
		::UpdateWindow(hWnd);
	}

	SystemParametersInfo(SPI_SETANIMATION, sizeof(state.animation), const_cast<ANIMATIONINFO*>(&state.animation), 0);
	SetLayeredWindowAttributes(hWnd, state.key, state.alpha, state.flags);
	SetWindowLong(hWnd, GWL_EXSTYLE, state.windowExStyle);
}

int CoverExtractor::getEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
	return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
	return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return 0;
}

CoverExtractor::Cover* CoverExtractor::createCoverFromStream(::IStream* stream)
{
	Cover* cover = 0;
	char* buffer = 0;

	do
	{
		LARGE_INTEGER location = {0};
		ULARGE_INTEGER coverSize;
		if (FAILED(stream->Seek(location, STREAM_SEEK_END, &coverSize)))
		{
			break;
		}

		if (!coverSize.QuadPart)
		{
			break;
		}

		buffer = new char[coverSize.LowPart];

		if (FAILED(stream->Seek(location, STREAM_SEEK_SET, 0)))
		{
			break;
		}

		ULONG readCount;
		if (FAILED(stream->Read(buffer, coverSize.LowPart, &readCount)))
		{
		}

		if (readCount != coverSize.LowPart)
		{
			break;
		}

		cover = new Cover;
		cover->image = buffer;
		cover->length = coverSize.LowPart;
	}
	while (0);

	if (!cover)
	{
		delete [] buffer;
	}

	return cover;
}

CoverExtractor::MemoryStream::~MemoryStream()
{
	delete [] m_buffer;
}

	// IUnknown

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::QueryInterface(REFIID iid, void ** ppvObject)
{ 
    if (iid == __uuidof(IUnknown)
        || iid == __uuidof(IStream)
        || iid == __uuidof(ISequentialStream))
    {
        *ppvObject = static_cast<IStream*>(this);
        AddRef();
        return S_OK;
    } else
        return E_NOINTERFACE; 
}

ULONG STDMETHODCALLTYPE CoverExtractor::MemoryStream::AddRef(void)
{ 
	return 1;
}

ULONG STDMETHODCALLTYPE CoverExtractor::MemoryStream::Release(void)
{
	return 1;
}

// ISequentialStream

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	ULONG maxRead = static_cast<ULONG>(cb > (m_size - m_offset) ? (m_size - m_offset) : cb);

	::memcpy_s(pv, cb, m_buffer, maxRead);
	m_offset += maxRead;

	if (pcbRead)
	{
		*pcbRead = maxRead;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	ULONG maxWrite = static_cast<ULONG>(cb > (m_capacity - m_offset) ? (m_capacity - m_offset) : cb);
	if (maxWrite != cb)
	{
		m_capacity = m_size + cb * 2;
		unsigned char* newBuffer = new unsigned char[static_cast<size_t>(m_capacity)];
		::memcpy(newBuffer, m_buffer, static_cast<size_t>(m_size));

		delete [] m_buffer;
		m_buffer = newBuffer;
	}

	::memcpy_s(m_buffer + m_offset, static_cast<size_t>(m_capacity - m_offset), pv, cb);
	m_offset += cb;
	m_size = m_size < m_offset ? m_offset : m_size;

	if (pcbWritten)
	{
		*pcbWritten = cb;
	}

	return S_OK;
}

// IStream

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::SetSize(ULARGE_INTEGER)
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
    ULARGE_INTEGER*) 
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Commit(DWORD)
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Revert(void)
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Clone(IStream **)
{ 
    return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
    ULARGE_INTEGER* lpNewFilePointer)
{ 
    switch(dwOrigin)
    {
		case STREAM_SEEK_SET:
		{
			if ((liDistanceToMove.QuadPart < 0) || (liDistanceToMove.QuadPart > m_size))
			{
				return STG_E_INVALIDFUNCTION;
			}

			m_offset = liDistanceToMove.QuadPart;
		}
		break;

		case STREAM_SEEK_CUR:
		{
			if (((m_offset + liDistanceToMove.QuadPart) < 0) || (m_offset + liDistanceToMove.QuadPart) > m_size)
			{
				return STG_E_INVALIDFUNCTION;
			}

			m_offset += liDistanceToMove.QuadPart;
		}
		break;

		case STREAM_SEEK_END:
		{
			if ((liDistanceToMove.QuadPart > 0) || (liDistanceToMove.QuadPart < -m_size))
			{
				return STG_E_INVALIDFUNCTION;
			}

			m_offset = m_size + liDistanceToMove.QuadPart;
		}
		break;

		default: return STG_E_INVALIDFUNCTION;
    }

	if (lpNewFilePointer)
	{
		lpNewFilePointer->QuadPart = m_offset;
	}

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CoverExtractor::MemoryStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
{
    return E_NOTIMPL;
}

}
