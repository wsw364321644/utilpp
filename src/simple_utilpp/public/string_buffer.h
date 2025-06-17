/**
 *  string_buffer.h
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "simple_export_ppdefs.h"

class SIMPLE_UTIL_EXPORT FCharBuffer
{
public:
    FCharBuffer();
    FCharBuffer(FCharBuffer &&)noexcept;
    FCharBuffer(FCharBuffer &);
    FCharBuffer(const char *cstr);
    FCharBuffer(const char *cstr, size_t size);
    ~FCharBuffer();
    FCharBuffer &operator()(const char *cstr);

    friend void Swap(FCharBuffer& l, FCharBuffer& r);

    // template <typename T,
    //     class = typename std::enable_if<std::is_same<typename std::decay<T>::type, char>::value>::type>
    //     void Push(T&& c);
    void Append(const char * cstr);
    void Append(const char *cstr, size_t size);
    void Append(FCharBuffer &);
    void Append(int);
    void Append(double);
    bool Format(const char* format, ...);
    bool FormatAppend(const char* format, ...);
    bool VFormatAppend(const char* format, va_list vlist);
    // template <typename T,
    //     class = typename std::enable_if<std::is_same<typename std::decay<T>::type, FCharBuffer>::value>::type>
    // void Append(T&&);
    void ReverseAssign(const char* cstr, size_t size);
    template <typename T>
    void ReverseAssign(const T* first, const T* last) {
        ReverseAssign(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first));
    }
    void Assign(const char *cstr, size_t size);
    template <typename T>
    void Assign(const char* first, const char* last) {
        Assign(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first));
    }

    void Clear();
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


    void(*freeptr)(void *const block);
    void *(*mallocptr)(size_t _Size);
    uint64_t GetIncreasedSize()
    {
        return (bufSize == 0 ? 64 : bufSize) * 2;
    };
    uint64_t bufSize;
    char *pBuf;
    uint64_t cursor;
    uint64_t readCursor;
};
