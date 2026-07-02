#pragma once

#include <string>
#include <singleton.h>
#include <filesystem>
#include "dir_definition.h"
#include "string_convert.h"
#include "CharBuffer.h"
#include "simple_export_ppdefs.h"
class SIMPLE_UTIL_EXPORT FPathBuf :public TProvideThreadSingletonClass<FPathBuf> {
public:
    FPathBuf() {
    }
    const char* GetBuf() const {
        ToPath();
        return GetBufInternal();
    }
    std::string_view GetView() const {
        return std::string_view(GetBuf(), GetPathLen());
    }
    std::u8string_view GetU8View() const {
        return std::u8string_view((const char8_t*)GetBuf(), GetPathLen());
    }
    const wchar_t* GetBufW() const {
        ToPathW();
        return GetBufInternalW();
    }
    std::wstring_view GetViewW() const {
        return std::wstring_view(GetBufW(), GetPathLenW());
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
        memcpy(GetBufInternalW(), path, len * sizeof(wchar_t));
        GetBufInternalW()[len] = L'\0';
        PathLenW = len;
        OnPathChanged(true);
    }
    void ToPathW(bool force = false) const {
        if (WSynced && !force) {
            return;
        }
        const_cast<uint32_t&>(PathLenW) = U8ToU16Buf(GetBufInternal(), PathLen, GetBufInternalW(), PATH_MAX);
        GetBufInternalW()[PathLenW] = '\0';
        const_cast<bool&>(WSynced)=true;
    }
    void ToPath(bool force = false) const{
        if (Synced && !force) {
            return;
        }
        auto ptr=const_cast<wchar_t*>(GetBufW());
        auto wptr=const_cast<wchar_t*>(GetBufW());
        const_cast<uint32_t&>(PathLen) = U16ToU8Buf(GetBufInternalW(), PathLenW, GetBufInternal(), PATH_MAX);
        GetBufInternal()[PathLen] = '\0';
        const_cast<bool&>(Synced) = true;
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
        ToPath();
        return Buf + PATH_PREFIX_MAX;
    }
    const wchar_t* GetPrependFileNamespacesW() {
        ToPathW();
        return BufW + PATH_PREFIX_MAX;
    }
#else
    const char* GetPrependFileNamespaces() {
        ToPath();
        if (CurrentPrifix != FILE_NAMESPACES) {
            memcpy(Buf + PATH_PREFIX_MAX - std::char_traits<char8_t>::length(FILE_NAMESPACES), FILE_NAMESPACES, std::char_traits<char8_t>::length(FILE_NAMESPACES));
            CurrentPrifix = FILE_NAMESPACES;
            PathPrependLen = std::char_traits<char8_t>::length(FILE_NAMESPACES);
        }
        return Buf + PATH_PREFIX_MAX - PathPrependLen;
    }
    const wchar_t* GetPrependFileNamespacesW() {
        ToPathW();
        if (CurrentPrifixW != FILE_NAMESPACESW) {
            memcpy(BufW + PATH_PREFIX_MAX - std::char_traits<wchar_t>::length(FILE_NAMESPACESW), FILE_NAMESPACESW, std::char_traits<wchar_t>::length(FILE_NAMESPACESW));
            CurrentPrifixW = FILE_NAMESPACESW;
            PathPrependLen = std::char_traits<wchar_t>::length(FILE_NAMESPACESW);
        }
        return BufW + PATH_PREFIX_MAX - PathPrependLen;
    }
#endif
#endif

    char* GetBufInternal() const {
        return const_cast<char*>(Buf) + PATH_PREFIX_MAX;
    }
    void UpdatePathLen(size_t newSize = std::numeric_limits<size_t>::max()) {
        if (newSize == std::numeric_limits<size_t>::max()) {
            newSize = GetStringLength(GetBuf());
        }
        if (newSize> std::char_traits<char8_t>::length(FILE_NAMESPACES)) {
            if (std::char_traits<char8_t>::compare(FILE_NAMESPACES, (const char8_t*)GetBufInternal(), std::char_traits<char8_t>::length(FILE_NAMESPACES)) == 0) {
                newSize -= std::char_traits<char8_t>::length(FILE_NAMESPACES);
                std::char_traits<char8_t>::move((char8_t*)GetBufInternal(), (char8_t*)GetBufInternal() + std::char_traits<char8_t>::length(FILE_NAMESPACES), newSize);
            }
        }
        PathLen = newSize;
        OnPathChanged(false);
    }

    wchar_t* GetBufInternalW() const {
        return  const_cast<wchar_t*>(BufW) + PATH_PREFIX_MAX;
    }
    void UpdatePathLenW(size_t newSize = std::numeric_limits<size_t>::max()) {
        if (newSize == std::numeric_limits<size_t>::max()) {
            newSize = GetStringLengthW(GetBufW());
        }
        if (newSize > std::char_traits<wchar_t>::length(FILE_NAMESPACESW)) {
            if (std::char_traits<wchar_t>::compare(FILE_NAMESPACESW, GetBufInternalW(), std::char_traits<wchar_t>::length(FILE_NAMESPACESW)) == 0) {
                newSize -= std::char_traits<wchar_t>::length(FILE_NAMESPACESW);
                std::char_traits<wchar_t>::move(GetBufInternalW(), GetBufInternalW() + std::char_traits<wchar_t>::length(FILE_NAMESPACESW), newSize);
            }
        }
        PathLenW = newSize;
        OnPathChanged(true);
    }

    uint32_t GetPathLen() const {
        ToPath();
        return PathLen;
    }
    uint32_t GetPathLenW() const {
        ToPathW();
        return PathLenW;
    }

    void AppendPath(std::string_view view) {
        AppendPath(view.data(), view.size());
    }
    void AppendPath(std::u8string_view view) {
        AppendPath(ConvertU8ViewToView(view));
    }
    void AppendPath(const char* path, size_t len) {
        ToPath();
        if (GetBufInternal()[PathLen - 1] != '\\' && GetBufInternal()[PathLen - 1] != '/') {
            GetBufInternal()[PathLen++] = static_cast<char>(std::filesystem::path::preferred_separator);
        }
        if (!path || len == 0) {
            return;
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
        ToPathW();
        if (GetBufInternalW()[PathLenW - 1] != L'\\' && GetBufInternalW()[PathLenW - 1] != L'/') {
            GetBufInternalW()[PathLenW++] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
        }
        if (!path || len == 0) {
            return;
        }
        memcpy(GetBufInternalW() + PathLenW, path, len * sizeof(wchar_t));
        PathLenW += len;
        GetBufInternalW()[PathLenW] = L'\0';
        OnPathChanged(true);
    }

    std::u8string_view PopPath(uint32_t depth = 1, bool bRemove = true) {
        ToPath();
        char* lastCursor{ nullptr };
        for (int i = int(PathLen) - 1; i >= 0; i--) {
            if (depth == 0) {
                break;
            }
            if (GetBufInternal()[i] == static_cast<char>(std::filesystem::path::preferred_separator)) {
                lastCursor = GetBufInternal() + i;
                depth--;
            }
        }

        if (lastCursor == NULL) {
            return std::u8string_view();
        }
        char* endCursor = GetBufInternal() + PathLen;
        if (bRemove) {
            *lastCursor = '\0';
            PathLen = lastCursor - GetBufInternal();
            OnPathChanged(false);
        }
        return ConvertViewToU8View(std::string_view(lastCursor + 1, endCursor));
    }
    std::u16string_view PopPathW(uint32_t depth = 1, bool bRemove = true) {
        ToPathW();
        wchar_t* lastCursor{ nullptr };
        for (int i = int(PathLenW) - 1; i >= 0; i--) {
            if (depth == 0) {
                break;
            }
            if (GetBufInternalW()[i] == static_cast<wchar_t>(std::filesystem::path::preferred_separator)) {
                lastCursor = GetBufInternalW() + i;
                depth--;
            }
        }
        if (lastCursor == NULL) {
            return std::u16string_view();
        }
        wchar_t* endCursor = GetBufInternalW() + PathLenW;
        if (bRemove) {
            *lastCursor = L'\0';
            PathLenW = lastCursor - GetBufInternalW();
            OnPathChanged(true);
        }
        return ConvertWViewToU16View(std::wstring_view(lastCursor + 1, endCursor));
    }

    bool PopRootPath(FCharBuffer* buf = nullptr) {
        ToPath();
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
        ToPathW();
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

    const char* FileName() {
        ToPath();
        const char* lastCursor = strrchr(GetBuf(), static_cast<char>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return nullptr;
        }
        return lastCursor + 1;
    }
    const wchar_t* FileNameW() {
        ToPathW();
        const wchar_t* lastCursor = wcsrchr(GetBufW(), static_cast<wchar_t>(std::filesystem::path::preferred_separator));
        if (lastCursor == NULL) {
            return nullptr;
        }
        return lastCursor + 1;
    }

    //bool IsFolder() {
    //    ToPath();
    //    if (GetBuf()[PathLen] == '/' || GetBuf()[PathLen] == '\\') {
    //        return true;
    //    }
    //    return false;
    //}
    //bool IsFolderW() {
    //    ToPathW();
    //    if (GetBufW()[PathLenW] == L'/' || GetBufW()[PathLen] == L'\\') {
    //        return true;
    //    }
    //    return false;
    //}
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

    char* CurrentPrifix{ nullptr };
    wchar_t* CurrentPrifixW{ nullptr };
    char Buf[PATH_MAX]{ 0 };
    wchar_t BufW[PATH_MAX]{ 0 };
    uint32_t PathLen{ 0 };
    uint32_t PathLenW{ 0 };
    uint32_t PathPrependLen{ 0 };
    bool Synced{ true };
    bool WSynced{ true };
};