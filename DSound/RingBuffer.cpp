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

#include "RingBuffer.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

namespace dsbridge
{

RingBuffer::RingBuffer()
: m_write(0)
, m_read(0)
, m_begin(0)
, m_end(0)
, m_dataAvailable(0)
{
}

RingBuffer::~RingBuffer()
{
	destroy();
}

bool RingBuffer::create(size_t size)
{
	m_begin = new char[size];
	if (!m_begin)
		return false;

	m_end = m_begin + size;

	m_write = m_begin;
	m_read = m_begin;

	m_dataAvailable = 0;

	return true;
}

void RingBuffer::destroy()
{
	delete [] m_begin;

	m_begin = 0;
	m_end = 0;

	m_write = 0;
	m_read = 0;

	m_dataAvailable = 0;
}

size_t RingBuffer::written() const
{
	return m_dataAvailable;
}

size_t RingBuffer::left() const
{
	return (m_end-m_begin) - written();
}

bool RingBuffer::write(const void* buffer, size_t size)
{
	if (!size)
		return true;

	if (left() < size)
	{
		return false;
	}

	if ((m_write >= m_read) && (size > size_t(m_end - m_write)))
	{
		const char* localBuffer = static_cast<const char*>(buffer);
		size_t endWrite = m_end-m_write;

		::memcpy(m_write, localBuffer, endWrite);
		::memcpy(m_begin, localBuffer + endWrite, size - endWrite);
		m_write = m_begin + (size - endWrite);
	}
	else
	{
		::memcpy(m_write,buffer,size);
		m_write += size;
	}

	m_dataAvailable += size;
	if (m_write == m_end)
		m_write = m_begin;

	return true;
}

size_t RingBuffer::read(void* buffer, size_t size)
{
	if (!size)
		return 0;

	char* localBuffer = static_cast<char*>(buffer);
	size_t actual = size > written() ? written() : size;

	if ((m_read >= m_write) && (actual > size_t(m_end - m_read)))
	{
		size_t endRead = m_end-m_read;

		::memcpy(localBuffer,m_read,endRead);
		::memcpy(localBuffer+endRead,m_begin,actual-endRead);
		m_read = m_begin + (actual-endRead);
	}
	else
	{
		::memcpy(localBuffer,m_read,actual);
		m_read += actual;
	}

	m_dataAvailable -= actual;
	if (m_read == m_end)
		m_read = m_begin;

	return actual;
}

size_t RingBuffer::peek(void* buffer, size_t size, size_t offset)
{
	if (!size)
		return 0;

	char* localBuffer = static_cast<char*>(buffer);
	size_t actual = size > written() ? written() : size;

	if ((m_read >= m_write) && (actual > size_t(m_end - m_read)))
	{
		size_t endRead = m_end-m_read;

		::memcpy(localBuffer,m_read,endRead);
		::memcpy(localBuffer+endRead,m_begin,actual-endRead);
	}
	else
	{
		::memcpy(localBuffer,m_read,actual);
	}

	return actual;
}

size_t RingBuffer::seek(size_t size)
{
	if (!size)
		return 0;

	size_t maxSeek = written();
	size_t actual = maxSeek < size ? maxSeek : size;

	if ((m_read >= m_write) && (actual > size_t(m_end - m_read)))
	{
		size_t endRead = m_end-m_read;
		m_read = m_begin + (actual-endRead);
	}
	else
	{
		m_read += actual;
	}

	m_dataAvailable -= actual;
	if (m_read == m_end)
		m_read = m_begin;

	return actual;
}

void RingBuffer::discard(size_t size)
{
	size_t maxDiscard = written();
	size_t actual = size > maxDiscard ? maxDiscard : size;

	if ((m_write <= m_read) && (actual > size_t(m_write - m_begin)))
	{
		size_t beginWrite = m_write - m_begin;
		m_write = m_read + (actual - beginWrite);
	}
	else
	{
		m_write -= actual;
	}

	m_dataAvailable -= actual;
}

}
