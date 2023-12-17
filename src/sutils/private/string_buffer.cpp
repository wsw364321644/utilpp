#include <string_buffer.h>
#include <string>
#include <stdarg.h>
#include <vector>
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

void Swap(CharBuffer& l, CharBuffer& r) {
    std::swap(l.bufSize,r.bufSize);
    std::swap(l.cursor,r.cursor);
    std::swap(l.readCursor,r.readCursor);
    std::swap(l.pBuf,r.pBuf);
    std::swap(l.mallocptr,r.mallocptr);
    std::swap(l.freeptr,r.freeptr);
}

void CharBuffer::Append(const char* cstr)
{
    Append(cstr, strlen(cstr));
}

void CharBuffer::Append(const char *str, size_t size)
{
    if(size <= 0) {
        return;
    }
    if (bufSize == 0 || size + cursor > bufSize - 1)
    {
        Reverse(GetIncreasedSize() > size + cursor+1 ? GetIncreasedSize() : size + cursor+1);
    }
    memcpy(pBuf + cursor, str, size);
    cursor += size;
}

void CharBuffer::Append(CharBuffer &inbuf)
{
    Append(inbuf.CStr(), inbuf.Length());
}

void CharBuffer::Append(int num)
{
    Append(std::to_string(num).c_str());
}

void CharBuffer::Append(double num)
{
    Append(std::to_string(num).c_str());
}

bool CharBuffer::FormatAppend(const char* format, ...)
{
    // initializing list pointer 
    va_list ptr;
    va_start(ptr, format);

    auto oldcursor = cursor;
    // char array to store token 
    const char* head = format;
    // parsing the formatted string 
    for (int i = 0 ; ; ) {
        if (format[i] == '%' || format[i] == '\0') {
            Append(head, format + i - head);
            if (format[i] == '%') {
                int j = i+1;
                char ch1 = 0;

                // this loop is required when printing 
                // formatted value like 0.2f, when ch1='f' 
                // loop ends 
                while ((ch1 = format[j++]) < 58) {
                }
                // for integers 
                if (ch1 == 'i' || ch1 == 'd' || ch1 == 'u'
                    || ch1 == 'h') {
                    auto num=va_arg(ptr, int);
                    Append(std::to_string(num).c_str());
                }
                // for characters 
                else if (ch1 == 'c') {
                    char num = va_arg(ptr, int);
                    Put(num);
                }
                // for float values 
                else if (ch1 == 'f') {
                    auto num = va_arg(ptr, double);
                    Append(std::to_string(num).c_str());
                }
                else if (ch1 == 'l') {
                    char ch2 = format[j++];
                    // for long int 
                    if (ch2 == 'u') {
                        auto num = va_arg(ptr, uint64_t);
                        Append(std::to_string(num).c_str());
                    }
                    else if (ch2 == 'd'
                        || ch2 == 'i') {
                        auto num = va_arg(ptr, int64_t);
                        Append(std::to_string(num).c_str());
                    }
                    // for double 
                    else if (ch2 == 'f') {
                        auto num = va_arg(ptr, double);
                        Append(std::to_string(num).c_str());
                    }
                    else {
                        cursor= oldcursor;
                        return false;
                    }
                }
                else if (ch1 == 'L') {
                    char ch2 = format[j++];

                    // for long long int 
                    if (ch2 == 'u' || ch2 == 'd'
                        || ch2 == 'i') {
                        auto num = va_arg(ptr, long long);
                        Append(std::to_string(num).c_str());
                    }

                    // for long double 
                    else if (ch2 == 'f') {
                        auto num = va_arg(ptr, long double);
                        Append(std::to_string(num).c_str());
                    }
                    else {
                        cursor = oldcursor;
                        return false;
                    }
                }

                // for strings 
                else if (ch1 == 's') {
                    auto str = va_arg(ptr, char*);
                    Append(str);
                }

                // print the whole token 
                // if no case is matched 
                else {
                    Append(head, format + i - head);
                }
                head = format + j;
                i = j;
            }
            else {
                break;
            }
        }
        else {
            i++;
        }
    }

    // ending traversal 
    va_end(ptr);
    return true;
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
