#pragma once
#include <stdint.h>
#include <string>
#include <stddef.h>
#include <singleton.h>
#include "simple_export_ppdefs.h"

class SIMPLE_UTIL_EXPORT FCharBuffer :public TProvideThreadSingletonClass<FCharBuffer>
{
public:
    FCharBuffer();
    FCharBuffer(size_t size);
    FCharBuffer(FCharBuffer &&)noexcept;
    FCharBuffer(const FCharBuffer &);
    FCharBuffer(const char *cstr);
    FCharBuffer(const char *cstr, size_t size);
    ~FCharBuffer();
    FCharBuffer& operator()(const char *cstr);
    FCharBuffer& operator=(FCharBuffer&& r) noexcept;
    FCharBuffer& operator=(const FCharBuffer& r)noexcept;
    char& operator[](const size_t pos)noexcept;
    friend void Swap(FCharBuffer& l, FCharBuffer& r);

    // template <typename T,
    //     class = typename std::enable_if<std::is_same<typename std::decay<T>::type, char>::value>::type>
    //     void Push(T&& c);
    void Append(const char *cstr, size_t size);
    void Append(std::string_view);
    void Append(FCharBuffer &);
    void Append(int);
    void Append(double);
    bool Format(const char* format, ...);
    bool FormatAppend(const char* format, ...);
    bool VFormatAppend(const char* format, va_list vlist);
    void Reverse(uint32_t size);
    void Resize(uint32_t size);
    void ReverseAssign(const char* cstr, size_t size);
    void ReverseAssign(std::string_view view);
    template <typename T>
    void ReverseAssign(const T first, const T last) {
        ReverseAssign(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first));
    }
    void Assign(const char *cstr, size_t size);
    void Assign(std::string_view view);
    template <typename T>
    void Assign(T first, T last) {
        Assign(reinterpret_cast<const char*>(first), reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first));
    }

    void Clear();
    const char *CStr();
    std::string_view View() const;
    char *Data() const;
    void SetLength(size_t);
    size_t Length() const;
    size_t Size() const;
    size_t Capacity() const;
    FCharBuffer& Seekg(size_t);
    FCharBuffer& Seekp(size_t);

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

    //! Begin writing operation at the current read pointer.
    //! \return The begin writer pointer.
    Ch* PutBegin() { return nullptr; }
    //! End the writing operation.
    //! \param begin The begin write pointer returned by PutBegin().
    //! \return Number of characters written.
    size_t PutEnd(Ch* begin) { return NULL; }



private:


    void(*freeptr)(void* const block) { nullptr };
    void* (*mallocptr)(size_t _Size) { nullptr };
    uint64_t GetIncreasedSize()
    {
        return (bufSize == 0 ? 64 : bufSize) * 2;
    };
    uint64_t bufSize{ 0 };
    char* pBuf{nullptr};
    uint64_t cursor{ 0 };
    uint64_t readCursor{ 0 };
};
