#pragma once

#include <string>
#include <optional>
#include <filesystem>
#include "dir_definition.h"
#include "string_convert.h"
#include "CharBuffer.h"
#include "simple_export_ppdefs.h"
class SIMPLE_UTIL_EXPORT FPathBuf{
public:
    FPathBuf() {
    }
    const char* GetBuf() const {
        return Buf + PATH_PREFIX_MAX;
    }
    const wchar_t* GetBufW() const {
        return BufW + PATH_PREFIX_MAX;
    }
    void SetPath(std::string_view view) {
        SetPath(view.data(), view.size());
    }
    void SetPath(const char* path, size_t len) {
        memcpy(GetBufInternal(), path, len);
        GetBufInternal()[len] = '\0';
        PathLen = len;
        OnPathChanged(false);
    }
    void SetPathW(std::wstring_view view) {
        SetPathW(view.data(), view.size());
    }
    void SetPathW(const wchar_t* path, size_t len) {
        memcpy(GetBufInternalW(), path, len*sizeof(wchar_t));
        GetBufInternalW()[len] = L'\0';
        PathLenW = len;
        OnPathChanged(true);
    }
    void ToPathW(bool force=false) {
        if (WSynced && !force) {
            return;
        }
        PathLenW=U8ToU16Buf(GetBufInternal(), PathLen, GetBufInternalW(), PATH_MAX);
        GetBufInternalW()[PathLenW] = '\0';
        OnPathSynced();
    }
    void ToPath(bool force = false) {
        if (Synced && !force) {
            return;
        }
        PathLen=U16ToU8Buf(GetBufInternalW(), PathLenW, GetBufInternal(), PATH_MAX);
        GetBufInternal()[PathLen] = '\0';
        OnPathSynced();
    }

    template<typename T>
    void SetNormalizePath(const T* ptr, size_t len) {
        std::filesystem::path p;
        if (sizeof(T) == sizeof(char8_t)) {
            new (&p) std::filesystem::path(std::u8string_view((const char8_t*)ptr, len));
        }
        else {
            new (&p) std::filesystem::path(std::u16string_view((const char16_t*)ptr, len));
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
            new (&p) std::filesystem::path(std::u16string_view((const char16_t*)ptr, len));
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
        return const_cast<char*>(GetBuf());
    }
    void UpdatePathLen(size_t newSize=std::numeric_limits<size_t>::max()) {
        if (newSize == std::numeric_limits<size_t>::max()) {
            PathLen = GetStringLength(GetBuf());
        }
        else {
            PathLen = newSize;
        }
        OnPathChanged(false);
    }
    wchar_t* GetBufInternalW() {
        return const_cast<wchar_t*>(GetBufW());
    }
    void UpdatePathLenW(size_t newSize = std::numeric_limits<size_t>::max()) {
        if (newSize == std::numeric_limits<size_t>::max()) {
            PathLenW = GetStringLengthW(GetBufW());
        }
        else {
            PathLenW = newSize;
        }
        OnPathChanged(true);
    }
    void AppendPath(std::string_view view) {
        AppendPath(view.data(), view.size());
    }
    void AppendPath(const char* path, size_t len) {
        if (GetBufInternal()[PathLen-1] !='\\'&& GetBufInternal()[PathLen-1] != '/') {
            GetBufInternal()[PathLen++] = static_cast<char>(std::filesystem::path::preferred_separator);
        }
        memcpy(GetBufInternal() + PathLen, path, len);
        PathLen += len;
        GetBufInternal()[PathLen] = '\0';
        OnPathChanged(false);
    }
    void AppendPathW(std::wstring_view view) {
        AppendPathW(view.data(), view.size());
    }
    void AppendPathW(const wchar_t* path, size_t len) {
        if (GetBufInternalW()[PathLenW -1] != L'\\' && GetBufInternalW()[PathLenW -1] != L'/') {
            GetBufInternalW()[PathLenW++] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
        }
        memcpy(GetBufInternalW() + PathLenW, path, len * sizeof(wchar_t));
        PathLenW += len;
        GetBufInternalW()[PathLenW] = L'\0';
        OnPathChanged(true);
    }

    std::u8string_view PopPath() {
        char* lastCursor = strrchr(GetBufInternal() , static_cast<char>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return std::u8string_view();
        }
        *lastCursor = '\0';
        char* endCursor = GetBufInternal() + PathLen;
        PathLen = lastCursor - GetBufInternal();
        OnPathChanged(false);
        return ConvertViewToU8View(std::string_view(lastCursor + 1, endCursor));
    }
    std::u16string_view PopPathW() {
        wchar_t* lastCursor = wcsrchr(GetBufInternalW() , static_cast<wchar_t>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return std::u16string_view();
        }
        *lastCursor = L'\0';
        wchar_t* endCursor = GetBufInternalW() + PathLenW;
        PathLenW = lastCursor - GetBufInternalW();
        OnPathChanged(true);
        return ConvertWViewToU16View(std::wstring_view(lastCursor + 1, endCursor));
    }

    bool PopRootPath(FCharBuffer* buf=nullptr) {
        char* firstCursor = strchr(GetBufInternal(), static_cast<char>(std::filesystem::path::preferred_separator));
        if (firstCursor == NULL) {
            return false;
        }
        PathLen = PathLen - (firstCursor - GetBufInternal()) - 1;
        if (buf) {
            buf->Assign(GetBufInternal(), firstCursor);
        }
        memcpy(GetBufInternal(), firstCursor + 1, PathLen + 1);
        OnPathChanged(false);
        return true;
    }
    bool PopRootPathW(FCharBuffer* buf = nullptr) {
        wchar_t* firstCursor = wcschr(GetBufInternalW(), static_cast<wchar_t>(std::filesystem::path::preferred_separator));
        if (firstCursor == NULL) {
            return false;
        }
        PathLenW = PathLenW - (firstCursor - GetBufInternalW()) - 1;
        if (buf) {
            buf->Assign(GetBufInternalW(), firstCursor);
        }
        memcpy(GetBufInternal(), firstCursor + 1, (PathLenW + 1) * sizeof(wchar_t));
        OnPathChanged(true);
        return true;
    }

    const wchar_t* FileNameW() const {
        const wchar_t* lastCursor = wcsrchr(GetBufW(), static_cast<wchar_t>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return nullptr;
        }
        return lastCursor + 1;
    }
    const char* FileName() const {
        const char* lastCursor = strrchr(GetBuf(), static_cast<char>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return nullptr;
        }
        return lastCursor + 1;
    }
    void OnPathChanged(bool isWide) {
        if (isWide) {
            Synced = false;
            WSynced = true;
        }
        else {
            Synced = true;
            WSynced = false;
        }
    }
    void OnPathSynced() {
        Synced = true;
        WSynced = true;
    }
    char* CurrentPrifix{nullptr};
    wchar_t* CurrentPrifixW{ nullptr };
    char Buf[PATH_MAX]{0};
    wchar_t BufW[PATH_MAX]{0};
    size_t PathLen{ 0 };
    size_t PathLenW{ 0 };
    size_t PathPrependLen{ 0 };
    bool Synced{ true };
    bool WSynced{ true };
};
