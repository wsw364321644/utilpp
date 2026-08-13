#pragma once
#include "simple_export_ppdefs.h"
#include <stdint.h>
#include <string>
#include <system_error>
class SIMPLE_UTIL_EXPORT IFileBackedBuffer {
public:
    virtual ~IFileBackedBuffer() {};
    virtual bool Init(uint32_t size, std::u8string_view fileName, std::error_code& ec) = 0;
    virtual void Close() = 0;
    virtual bool Clean(std::error_code& ec) = 0;
    /// @brief write data to disk
    virtual void IOTick(float delSec) = 0;
    virtual void* GetPtr(uint32_t offset)const = 0;
    virtual void WriteData(void* target, uint8_t& val) = 0;
    virtual void WriteData(void* target, uint16_t& val) = 0;
    virtual void WriteData(void* target, uint32_t& val) = 0;
    virtual void WriteData(void* target, uint64_t& val) = 0;
    virtual void WriteData(void* target, void* source, uint32_t size) = 0;

    template<class T>
    const T& GetData(uint32_t offset) const {
        return *(T*)GetPtr(offset);
    }

    template<class T>
    void WriteData(uint32_t offset, T&& value) {
        WriteData(GetPtr(offset), value);
    }

    template<class T>
    void WriteData(void* ptr, T&& value) {
        WriteInternal(ptr, value, std::integral_constant<size_t, sizeof(T)>());
    }

    template <typename T>
    void WriteInternal(void* ptr, T&& value, std::integral_constant<size_t, 1>) {
        WriteData(ptr, *(uint8_t*)&value);
    }

    template <typename T>
    void WriteInternal(void* ptr, T&& value, std::integral_constant<size_t, 2>) {
        WriteData(ptr, *(uint16_t*)&value);
    }

    template <typename T>
    void WriteInternal(void* ptr, T&& value, std::integral_constant<size_t, 4>) {
        WriteData(ptr, *(uint32_t*)&value);
    }

    template <typename T>
    void WriteInternal(void* ptr, T&& value, std::integral_constant<size_t, 8>) {
        WriteData(ptr, *(uint64_t*)&value);
    }

    template <typename T, size_t N>
    typename std::enable_if<N != 1 && N != 2 && N != 4 && N != 8, void>::type
        WriteInternal(void* ptr, T&& value, std::integral_constant<size_t, N>) {
        WriteData(ptr, (void*)&value, N);
    }

};

SIMPLE_UTIL_EXPORT IFileBackedBuffer* NewFileBackedBuffer();
SIMPLE_UTIL_EXPORT void FreeFileBackedBuffer(IFileBackedBuffer*);