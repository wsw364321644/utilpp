/**
 *  string_buffer.h
 */
#pragma once
#include <stdint.h>
#include "simple_export_ppdefs.h"

class SIMPLE_UTIL_EXPORT CharBuffer
{
public:
    CharBuffer();
    CharBuffer(CharBuffer &&);
    CharBuffer(CharBuffer &);
    CharBuffer(const char *cstr);
    CharBuffer(const char *cstr, size_t size);
    ~CharBuffer();
    CharBuffer &operator()(const char *cstr);

    friend void Swap(CharBuffer& l, CharBuffer& r);

    // template <typename T,
    //     class = typename std::enable_if<std::is_same<typename std::decay<T>::type, char>::value>::type>
    //     void Push(T&& c);
    void Append(const char * cstr);
    void Append(const char *cstr, size_t size);
    void Append(CharBuffer &);
    void Append(int);
    void Append(double);
    bool FormatAppend(const char* format, ...);
    
    // template <typename T,
    //     class = typename std::enable_if<std::is_same<typename std::decay<T>::type, CharBuffer>::value>::type>
    // void Append(T&&);

    void Assign(const char *cstr, size_t size);
    void Clear() { cursor = 0; }

    const char *CStr() const;
    char *Data();
    size_t Length() const;
    size_t Size() const;
    typedef char Ch;

    //! Read the current character from stream without moving the read cursor.
    Ch Peek() const;

    //! Read the current character from stream and moving the read cursor to next character.
    Ch Take();

    //! Get the current read cursor.
    //! \return Number of characters read from start.
    size_t Tell();

    //! Write a character.
    void Put(Ch c);

    //! Flush the buffer.
    void Flush(){};

    void Reverse(uint32_t size);
    void Resize(uint32_t size);

private:


    void(__cdecl *freeptr)(void *const block);
    void *(__cdecl *mallocptr)(size_t _Size);
    uint32_t GetIncreasedSize()
    {
        return (bufSize == 0 ? 64 : bufSize) * 2;
    };
    uint32_t bufSize;
    char *pBuf;
    uint32_t cursor;
    uint32_t readCursor;
};
