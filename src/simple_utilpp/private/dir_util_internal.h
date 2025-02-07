#pragma once
#include "string_convert.h"
#include <string>
#include <limits>
#include <filesystem>
#ifdef _WIN32
constexpr char8_t FILE_NAMESPACES[] = u8"\\\\?\\";
constexpr wchar_t FILE_NAMESPACESW[] = L"\\\\?\\";
constexpr size_t PATH_PREFIX_MAX = std::char_traits<char8_t>::length(FILE_NAMESPACES);
constexpr size_t PATH_MAX = std::numeric_limits<int16_t>::max();
#else
constexpr size_t PATH_PREFIX_MAX = 0;
#endif // _WIN32


class FPathBuf{
public:
    FPathBuf() {
    }
    const char* GetBuf() {
        return GetBufInternal();
    }
    const wchar_t* GetBufW() {
        return GetBufInternalW();
    }

    void SetPath(const char* path, size_t len) {
        memcpy(GetBufInternal(), path, len);
        GetBufInternal()[len] = '\0';
        PathLen = len;
    }
    void SetPathW(const wchar_t* path, size_t len) {
        memcpy(GetBufInternalW(), path, len*sizeof(wchar_t));
        GetBufInternalW()[len] = L'\0';
        PathLenW = len;
    }
    void ToPathW() {
        U8ToU16Buf(GetBufInternal(), PathLen, GetBufInternalW(), PATH_MAX);
    }
    void ToPath() {
        U16ToU8Buf(GetBufInternalW(), PathLenW, GetBufInternal(), PATH_MAX);
    }

    template<typename T>
    void SetNormalizePath(const T* ptr, size_t len) {
        std::filesystem::path p;
        if (sizeof(T) == sizeof(char8_t)) {
            new (&p) std::filesystem::path(std::u8string_view((const char8_t*)ptr, len));
        }
        else {
            new (&p) std::filesystem::path(std::u8string_view((const char8_t*)ptr, len));
        }
        p = p.lexically_normal();
        SetPath((const char*)p.u8string().c_str(), p.u8string().length());
    }
    template<typename T>
    void SetNormalizePathW(const T* ptr, size_t len) {
        std::filesystem::path p;
        if (sizeof(T) == sizeof(char8_t)) {
            new (&p) std::filesystem::path(std::u8string_view((const char8_t*)ptr, len));
        }
        else {
            new (&p) std::filesystem::path(std::u8string_view((const char8_t*)ptr, len));
        }
        p = p.lexically_normal();
        SetPathW((const wchar_t*)p.u16string().c_str(), p.u16string().length());
    }
#ifdef WIN32
#if WDK_NTDDI_VERSION >= NTDDI_WIN10_RS1
    const char* GetPrependFileNamespaces() {
        return Buf + PATH_PREFIX_MAX;
    }
    const wchar_t* GetPrependFileNamespacesW() {
        return BufW + PATH_PREFIX_MAX;
    }
#else
    const char* GetPrependFileNamespaces() {
        if (CurrentPrifix != FILE_NAMESPACES) {
            memcpy(Buf + PATH_PREFIX_MAX - std::char_traits<char8_t>::length(FILE_NAMESPACES), FILE_NAMESPACES, std::char_traits<char8_t>::length(FILE_NAMESPACES));
            CurrentPrifix = FILE_NAMESPACES;
            PathPrependLen = std::char_traits<char8_t>::length(FILE_NAMESPACES);
        }
        return Buf + PATH_PREFIX_MAX - PathPrependLen;
    }
    const wchar_t* GetPrependFileNamespacesW() {
        if (CurrentPrifixW != FILE_NAMESPACESW) {
            memcpy(BufW + PATH_PREFIX_MAX - std::char_traits<wchar_t>::length(FILE_NAMESPACESW), FILE_NAMESPACESW, std::char_traits<wchar_t>::length(FILE_NAMESPACESW));
            CurrentPrifixW = FILE_NAMESPACESW;
            PathPrependLen = std::char_traits<wchar_t>::length(FILE_NAMESPACESW);
        }
        return BufW + PATH_PREFIX_MAX - PathPrependLen;
    }
#endif
#endif

    char* GetBufInternal() {
        return Buf + PATH_PREFIX_MAX;
    }
    wchar_t* GetBufInternalW() {
        return BufW + PATH_PREFIX_MAX;
    }
    void AppendPath(const char* path, size_t len) {
        if (GetBufInternal()[PathLen-1] !='\\'&& GetBufInternal()[PathLen-1] != '/') {
            GetBufInternal()[PathLen++] = static_cast<char>(std::filesystem::path::preferred_separator);
        }
        memcpy(GetBufInternal() + PathLen, path, len);
        GetBufInternal()[PathLen++] = '0';
    }
    void AppendPathW(const wchar_t* path, size_t len) {
        if (GetBufInternalW()[PathLenW -1] != L'\\' && GetBufInternalW()[PathLenW -1] != L'/') {
            GetBufInternalW()[PathLenW++] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
        }
        memcpy(GetBufInternalW() + PathLenW, path, len * sizeof(wchar_t));
        GetBufInternalW()[PathLenW++] = L'0';
    }

    bool PopPath() {
        char* lastCursor = strrchr(GetBufInternal() , static_cast<char>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return false;
        }
        *lastCursor = '/0';
        PathLen = lastCursor - GetBufInternal();
        return true;
    }
    bool PopPathW() {
        wchar_t* lastCursor = wcsrchr(GetBufInternalW() , static_cast<wchar_t>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return false;
        }
        *lastCursor = L'/0';
        PathLenW = lastCursor - GetBufInternalW();
        return true;
    }
    char* CurrentPrifix{nullptr};
    wchar_t* CurrentPrifixW{ nullptr };
    char Buf[PATH_MAX]{0};
    wchar_t BufW[PATH_MAX]{0};
    size_t PathLen{ 0 };
    size_t PathLenW{ 0 };
    size_t PathPrependLen{ 0 };
};

extern thread_local FPathBuf PathBuf;
extern thread_local FPathBuf PathBuf2;