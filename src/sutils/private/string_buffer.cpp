/**
 *  util.h
 */

#pragma once

#include <string_buffer.h>
#include <string>

CharBuffer::CharBuffer() : bufSize(0), cursor(0), readCursor(0), pBuf(nullptr), freeptr(free), mallocptr(malloc)
{
}
CharBuffer::CharBuffer(CharBuffer &&mbuf) : CharBuffer()
{
    pBuf = mbuf.pBuf;
    mbuf.pBuf = nullptr;
    bufSize = mbuf.bufSize;
    cursor = mbuf.cursor;
    readCursor = mbuf.readCursor;
}
CharBuffer::CharBuffer(CharBuffer &mbuf) : CharBuffer()
{
    Assign(mbuf.CStr(), mbuf.Length());
}
CharBuffer::CharBuffer(const char *cstr) : CharBuffer()
{
    Assign(cstr, strlen(cstr));
}
CharBuffer::CharBuffer(const char *cstr, size_t size) : CharBuffer()
{
    Assign(cstr, size);
}
CharBuffer::~CharBuffer()
{
    if (pBuf)
    {
        freeptr(pBuf);
    }
}
CharBuffer &CharBuffer::operator()(const char *cstr)
{
    Assign(cstr, strlen(cstr));
    return *this;
}

void CharBuffer::Append(const char *str, size_t size)
{
    if (bufSize == 0 || size + cursor > bufSize - 1)
    {
        Reverse(GetIncreasedSize() > size + cursor ? GetIncreasedSize() : size + cursor);
    }
    memcpy(pBuf + cursor, str, size);
    cursor += size;
}

void CharBuffer::Append(CharBuffer &inbuf)
{
    Append(inbuf.CStr(), inbuf.Length());
}

void CharBuffer::Assign(const char *cstr, size_t size)
{
    Resize(size + 1);
    if (size > 0)
    {
        memcpy(pBuf, cstr, size);
    }
    cursor = size;
}

const char *CharBuffer::CStr() const
{
    pBuf[cursor] = 0;
    return pBuf;
}
char *CharBuffer::Data()
{
    return pBuf;
}
size_t CharBuffer::Length() const
{
    return cursor;
}

size_t CharBuffer::Size() const
{
    return bufSize;
}

char CharBuffer::Peek() const
{
    return pBuf[readCursor];
}

char CharBuffer::Take()
{
    return pBuf[readCursor++];
}

size_t CharBuffer::Tell()
{
    return readCursor;
}

void CharBuffer::Put(char c)
{
    if (bufSize == 0 || cursor >= bufSize - 1)
    {
        Reverse(GetIncreasedSize());
    }
    pBuf[cursor++] = c;
}
void CharBuffer::Reverse(uint32_t size)
{
    if (size <= bufSize)
    {
        return;
    }
    char *newbuf = (char *)mallocptr(size);
    if (bufSize > 0)
    {
        memcpy(newbuf, pBuf, cursor);
        freeptr(pBuf);
    }
    bufSize = size;
    pBuf = newbuf;
}

void CharBuffer::Resize(uint32_t size)
{
    if (size == 0)
    {
        if (bufSize > 0)
        {
            free(pBuf);
            bufSize = 0;
            pBuf = nullptr;
        }
    }
    char *newbuf = (char *)mallocptr(size);
    cursor = size > cursor ? cursor : size;
    if (bufSize > 0)
    {
        memcpy(newbuf, pBuf, cursor);
        freeptr(pBuf);
    }
    bufSize = size;
    pBuf = newbuf;
}
