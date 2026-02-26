#include <CharBuffer.h>
#include <cstring>
#include <string>
#include <stdarg.h>
#include <vector>
#include <FunctionExitHelper.h>

FCharBuffer::FCharBuffer() : bufSize(0), cursor(0), readCursor(0), pBuf(nullptr), freeptr(free), mallocptr(malloc)
{
}
FCharBuffer::FCharBuffer(size_t size) : FCharBuffer()
{
    Resize(size);
}
FCharBuffer::FCharBuffer(FCharBuffer &&mbuf)noexcept : FCharBuffer()
{
    Swap(*this, mbuf);
}
FCharBuffer::FCharBuffer(const FCharBuffer &buf) : FCharBuffer()
{
    operator=(buf);
}
FCharBuffer::FCharBuffer(const char *cstr) : FCharBuffer()
{
    Assign(cstr, strlen(cstr));
}
FCharBuffer::FCharBuffer(const char *cstr, size_t size) : FCharBuffer()
{
    Assign(cstr, size);
}
FCharBuffer::~FCharBuffer()
{
    if (pBuf)
    {
        freeptr(pBuf);
    }
}
FCharBuffer& FCharBuffer::operator()(const char *cstr)
{
    Assign(cstr, strlen(cstr));
    return *this;
}
FCharBuffer& FCharBuffer::operator=(FCharBuffer&& r)noexcept {
    Swap(*this, r);
    return *this;
}
FCharBuffer& FCharBuffer::operator=(const FCharBuffer& r)noexcept {
    freeptr = r.freeptr;
    mallocptr = r.mallocptr;
    Assign(r.Data(), r.Length());
    readCursor = r.readCursor;
    return *this;
}
char& FCharBuffer::operator[](const size_t pos) noexcept
{
    return Data()[pos];
}
void Swap(FCharBuffer& l, FCharBuffer& r) {
    std::swap(l.bufSize,r.bufSize);
    std::swap(l.cursor,r.cursor);
    std::swap(l.readCursor,r.readCursor);
    std::swap(l.pBuf,r.pBuf);
    std::swap(l.mallocptr,r.mallocptr);
    std::swap(l.freeptr,r.freeptr);
}

void FCharBuffer::Append(const char *str, size_t size)
{
    if(size <= 0) {
        return;
    }
    if (bufSize == 0 || size + cursor > bufSize - 1)
    {
        Reverse(std::max(GetIncreasedSize(), size + cursor + 2));
    }
    memcpy(pBuf + cursor, str, size);
    cursor += size;
}

void FCharBuffer::Append(std::string_view view) {
    Append(view.data(), view.size());
}
void FCharBuffer::Append(FCharBuffer &inbuf)
{
    Append(inbuf.CStr(), inbuf.Length());
}

void FCharBuffer::Append(int num)
{
    Append(std::to_string(num).c_str());
}

void FCharBuffer::Append(double num)
{
    Append(std::to_string(num).c_str());
}

bool FCharBuffer::Format(const char* format, ...)
{
    Clear();
    va_list args;
    va_start(args, format);
    auto res = VFormatAppend(format, args);
    va_end(args);
    return res;
}

bool FCharBuffer::FormatAppend(const char* format, ...)
{
    //// initializing list pointer 
    //va_list ptr;
    //va_start(ptr, format);

    //auto oldcursor = cursor;
    //// char array to store token 
    //const char* head = format;
    //// parsing the formatted string 
    //for (int i = 0 ; ; ) {
    //    if (format[i] == '\0') {
    //        Append(head, format + i - head);
    //    }else if (format[i] == '%') {
    //        Append(head, format + i - head);
    //        int j = i+1;
    //        char ch1 = 0;
    //        // this loop is required when printing 
    //        // formatted value like 0.2f, when ch1='f' 
    //        // loop ends 
    //        while ((ch1 = format[j++]) < 58) {
    //        }
    //        // for integers 
    //        if (ch1 == 'i' || ch1 == 'd' || ch1 == 'u'
    //            || ch1 == 'h') {
    //            auto num=va_arg(ptr, int);
    //            Append(std::to_string(num).c_str());
    //        }
    //        // for characters 
    //        else if (ch1 == 'c') {
    //            char num = va_arg(ptr, int);
    //            Put(num);
    //        }
    //        // for float values 
    //        else if (ch1 == 'f') {
    //            auto num = va_arg(ptr, double);
    //            Append(std::to_string(num).c_str());
    //        }
    //        else if (ch1 == 'l') {
    //            char ch2 = format[j++];
    //            // for long int 
    //            if (ch2 == 'u') {
    //                auto num = va_arg(ptr, uint64_t);
    //                Append(std::to_string(num).c_str());
    //            }
    //            else if (ch2 == 'd'
    //                || ch2 == 'i') {
    //                auto num = va_arg(ptr, int64_t);
    //                Append(std::to_string(num).c_str());
    //            }
    //            // for double 
    //            else if (ch2 == 'f') {
    //                auto num = va_arg(ptr, double);
    //                Append(std::to_string(num).c_str());
    //            }
    //            else {
    //                cursor= oldcursor;
    //                return false;
    //            }
    //        }
    //        else if (ch1 == 'L') {
    //            char ch2 = format[j++];

    //            // for long long int 
    //            if (ch2 == 'u' || ch2 == 'd'
    //                || ch2 == 'i') {
    //                auto num = va_arg(ptr, long long);
    //                Append(std::to_string(num).c_str());
    //            }

    //            // for long double 
    //            else if (ch2 == 'f') {
    //                auto num = va_arg(ptr, long double);
    //                Append(std::to_string(num).c_str());
    //            }
    //            else {
    //                cursor = oldcursor;
    //                return false;
    //            }
    //        }

    //        // for strings 
    //        else if (ch1 == 's') {
    //            auto str = va_arg(ptr, char*);
    //            Append(str);
    //        }

    //        // print the whole token 
    //        // if no case is matched 
    //        else {
    //            Append(head, format + i - head);
    //        }
    //        head = format + j;
    //        i = j;
    //    }
    //    else {
    //        i++;
    //    }
    //}

    //// ending traversal 
    //va_end(ptr);
    va_list args;
    va_start(args, format);
    auto res=VFormatAppend(format, args);
    va_end(args);
    return res;
}

bool FCharBuffer::VFormatAppend(const char* format, va_list vlist)
{
    va_list args;
    va_copy(args, vlist);
    FunctionExitHelper_t va_copy_helper(
        [&args]() {
            va_end(args);
        }
    );
    auto available = bufSize - cursor;
    // up to bufsz - 1 characters may be written, plus the null terminator
    // num not including the terminating null-byte
    auto num = vsnprintf(pBuf + cursor, available, format, args);
    if (num < 0) {
        return false;
    }
    if (num < available) {
        cursor += num;
        return true;
    }
    Reverse(std::max(GetIncreasedSize(), num + cursor + 2));
    available = bufSize - cursor;
    num = vsnprintf(pBuf + cursor, available, format, args);
    if (num < 0) {
        return false;
    }
    cursor += num;
    return true;
}

void FCharBuffer::ReverseAssign(const char* cstr, size_t size)
{
    Reverse(size + 2);
    if (size > 0)
    {
        memcpy(pBuf, cstr, size);
    }
    cursor = size;
}

void FCharBuffer::ReverseAssign(std::string_view view)
{
    ReverseAssign(view.data(), view.size());
}

void FCharBuffer::Assign(const char *cstr, size_t size)
{
    Resize(size + 2);
    if (size > 0)
    {
        memcpy(pBuf, cstr, size);
    }
    cursor = size;
}

void FCharBuffer::Assign(std::string_view view)
{
    Assign(view.data(), view.size());
}

void FCharBuffer::Clear()
{
    cursor = 0;
    readCursor = 0;
    if (pBuf)
    {
        pBuf[0] = 0;
    }
}

const char* FCharBuffer::CStr()
{
    if (cursor >= bufSize) {
        Reverse(cursor + 1);
    }
    pBuf[cursor] = 0;
    return pBuf;
}
std::string_view FCharBuffer::View() const
{
    return std::string_view(Data(),Length());
}
char *FCharBuffer::Data() const
{
    return pBuf;
}
void FCharBuffer::SetLength(size_t l)
{
    if (l <= bufSize) {
        cursor = l;
    }
}
size_t FCharBuffer::Length() const
{
    return cursor;
}

size_t FCharBuffer::Size() const
{
    return cursor;
}

size_t FCharBuffer::Capacity() const
{
    return bufSize;
}

FCharBuffer& FCharBuffer::Seekg(size_t pos)
{
    readCursor = pos < cursor ? pos : cursor;
    return *this;
}

FCharBuffer& FCharBuffer::Seekp(size_t pos)
{
    cursor = pos < bufSize ? pos : bufSize;
    return *this;
}

FCharBuffer::Ch FCharBuffer::Peek() const
{
    return pBuf[readCursor];
}

FCharBuffer::Ch FCharBuffer::Take()
{
    return pBuf[readCursor++];
}

size_t FCharBuffer::Tell()
{
    return readCursor;
}

void FCharBuffer::Put(FCharBuffer::Ch c)
{
    if (bufSize == 0 || cursor >= bufSize - 1)
    {
        Reverse(GetIncreasedSize());
    }
    pBuf[cursor++] = c;
}
void FCharBuffer::Reverse(uint32_t size)
{
    auto desiredSize = size;
    if (desiredSize <= bufSize)
    {
        return;
    }
    char *newbuf = (char *)mallocptr(desiredSize);
    if (bufSize > 0)
    {
        memcpy(newbuf, pBuf, cursor);
        freeptr(pBuf);
    }
    bufSize = desiredSize;
    pBuf = newbuf;
    readCursor = 0;
}

void FCharBuffer::Resize(uint32_t size)
{
    auto desiredSize = size;
    if (desiredSize == bufSize)
    {
        return;
    }
    char *newbuf = (char *)mallocptr(desiredSize);
    cursor = desiredSize > cursor ? cursor : size;
    if (bufSize > 0)
    {
        memcpy(newbuf, pBuf, cursor);
        freeptr(pBuf);
    }
    bufSize = desiredSize;
    pBuf = newbuf;
    readCursor = 0;;
}
