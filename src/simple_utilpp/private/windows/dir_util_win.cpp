/**
 *  dir_util.cpp
 * 
 * https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
 * These are the directory management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CreateDirectoryW, CreateDirectoryExW GetCurrentDirectoryW RemoveDirectoryW SetCurrentDirectoryW.
 * These are the file management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CopyFileW, CopyFile2, CopyFileExW, CreateFileW, CreateFile2, CreateHardLinkW, CreateSymbolicLinkW, DeleteFileW, FindFirstFileW, FindFirstFileExW, FindNextFileW, GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW, GetFullPathNameW, GetLongPathNameW, MoveFileW, MoveFileExW, MoveFileWithProgressW, ReplaceFileW, SearchPathW, FindFirstFileNameW, FindNextFileNameW, FindFirstStreamW, FindNextStreamW, GetCompressedFileSizeW, GetFinalPathNameByHandleW.
 */

#include "dir_util.h"
#include "dir_util_internal.h"
#include "logger_header.h"
#include <wchar.h>
#include <assert.h>
#include <stack>
#include <simple_os_defs.h>
#include <ctre-unicode.hpp>

thread_local DirUtil::IterateDirCallback cb;
thread_local DirEntry_t out;
bool InternalCreateDir(wchar_t* pathw, size_t prependlen,size_t len) {
    for (size_t i = prependlen; i < len; i++) {
        if (pathw[i] != static_cast<wchar_t>(std::filesystem::path::preferred_separator)) {
            continue;
        }
        pathw[i] = L'\0';
        DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
        if (attr != INVALID_FILE_ATTRIBUTES) {
            if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                return false;
            }
        }
        else {
            if (CreateDirectoryW((LPCWSTR)pathw, NULL) == 0) {
                return false;
            }
        }
        pathw[i] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
    }
    if (pathw[len] != static_cast<wchar_t>(std::filesystem::path::preferred_separator)) {
        DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
        if (attr != INVALID_FILE_ATTRIBUTES) {
            if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                return false;
            }
        }
        else {
            if (CreateDirectoryW((LPCWSTR)pathw, NULL) == 0) {
                return false;
            }
        }
    }
    return true;
}

bool DirUtil::SetWritable(std::u8string_view  path)
{
    if (!IsExist(path))
        return true;
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_READONLY) {
        attr &= ~FILE_ATTRIBUTE_READONLY;
        if (0 == SetFileAttributesW((LPCWSTR)pathw, attr)) {
            return false;
        }
    }
    return true;
}
bool DirUtil::IsExist(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

bool DirUtil::IsDirectory(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return ((attr != INVALID_FILE_ATTRIBUTES) && (attr&FILE_ATTRIBUTE_DIRECTORY));
}

bool DirUtil::IsRegular(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr&FILE_ATTRIBUTE_DIRECTORY));
}

uint64_t DirUtil::FileSize(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    uint64_t fs = 0;
    HANDLE fh = CreateFileW((LPCWSTR)pathw,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (fh != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size;
        if (GetFileSizeEx(fh, &size)) {
            fs = size.QuadPart;
        }

        CloseHandle(fh);
    }
    return fs;
}

bool DirUtil::CreateDir(std::u8string_view  path)
{
    PathBuf.SetNormalizePathW(path.data(), path.length());
    auto pathw = (wchar_t*)PathBuf.GetPrependFileNamespacesW();
    return InternalCreateDir(pathw, PathBuf.PathPrependLen, PathBuf.PathLenW);
}

F_HANDLE DirUtil::RecursiveCreateFile(std::u8string_view  path, uint32_t flag) {
    PathBuf.SetNormalizePathW(path.data(), path.length());
    auto pathw = (wchar_t*)PathBuf.GetPrependFileNamespacesW();
    auto rawpathw = (wchar_t*)PathBuf.GetBufInternalW();

    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            return NULL;
        }
    }

    F_HANDLE handle= CreateFileW((LPCWSTR)pathw,
            GENERIC_WRITE | GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            flag,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        return handle;
    }

    auto err = GetLastError();
    // Path not found? Create the directory
    if ((flag != UTIL_CREATE_ALWAYS && flag != UTIL_OPEN_ALWAYS) || err != ERROR_PATH_NOT_FOUND) {
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't open the file after create directory: {}. ErrorCode is {}", pathw, err);
        return handle;
    }

    //todo
    wchar_t* lastCursor=wcsrchr(rawpathw, static_cast<wchar_t>(std::filesystem::path::preferred_separator));
    if (lastCursor==NULL) {
        return handle;
    }
    if (!InternalCreateDir(pathw, PathBuf.PathPrependLen, lastCursor - rawpathw)){
        return handle;
    }
    for (size_t i = 0; i < PathBuf.PathLenW; i++) {
        if (rawpathw[i] != static_cast<wchar_t>(std::filesystem::path::preferred_separator)) {
            continue;
        }
        rawpathw[i] = L'\0';
        DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
        if (attr != INVALID_FILE_ATTRIBUTES) {
            if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                return NULL;
            }
        }
        else {
            if (CreateDirectoryW((LPCWSTR)pathw, NULL) == 0) {
                return NULL;
            }
        }
        rawpathw[i] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
    }
    handle = CreateFileW((LPCWSTR)pathw,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        flag,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't open the file after create directory: {}. ErrorCode is {}", pathw, err);
    }

    return handle;
}

bool DirUtil::Delete(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    return DeleteFileW((LPCWSTR)pathw) == TRUE;
}

bool DirUtil::Rename(std::u8string_view  oldpath, std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    PathBuf2.SetPath((char*)oldpath.data(), oldpath.size());
    PathBuf2.ToPathW();
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    auto path_oldw = PathBuf2.GetPrependFileNamespacesW();
    return MoveFileExW(path_oldw, pathw, MOVEFILE_COPY_ALLOWED| MOVEFILE_REPLACE_EXISTING| MOVEFILE_WRITE_THROUGH) == TRUE;
}

std::u8string_view DirUtil::AbsolutePath(std::u8string_view  path)
{
    PathBuf2.SetPath((char*)path.data(), path.size());
    PathBuf2.ToPathW();
    auto pathw = PathBuf2.GetBufW();
    auto length = GetFullPathNameW((LPCWSTR)pathw, (DWORD)PATH_MAX, PathBuf.GetBufInternalW(), NULL);
    PathBuf.PathLenW = length;
    PathBuf.ToPath();
    return (const char8_t*)PathBuf.GetBuf();
}

bool RecursiveIterateDir(uint32_t depth) {
    WIN32_FIND_DATAW wfd;
    PathBuf.AppendPathW(L"*", 1);
    auto pathw = PathBuf.GetPrependFileNamespacesW();
    HANDLE hf = FindFirstFileW(pathw, &wfd);
    if (INVALID_HANDLE_VALUE == hf) {
        return false;
    }
    PathBuf.PopPathW();
    do {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
            (wcscmp(wfd.cFileName, L".") == 0 ||wcscmp(wfd.cFileName, L"..") == 0)) {
            continue;
        }
        PathBuf.AppendPathW(wfd.cFileName, GetStringLengthW(wfd.cFileName));
        PathBuf.ToPath();
        out.Name = (char8_t*)PathBuf.GetBufInternal();
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            out.bDir = true;
            cb(out);
            if (depth > 0) {
                if (!RecursiveIterateDir(depth-1)) {
                    return false;
                }
            }
        }
        else {
            LARGE_INTEGER size;
            size.LowPart = wfd.nFileSizeLow;
            size.HighPart = wfd.nFileSizeHigh;
            out.Size = size.QuadPart;
            out.bDir = false;
            cb(out);
        }
        PathBuf.PopPathW();
    } while (FindNextFileW(hf, &wfd) != 0);
    FindClose(hf);
    return true;
}

bool DirUtil::IterateDir(std::u8string_view  path, IterateDirCallback _cb, uint32_t depth)
{
    PathBuf.SetNormalizePathW(path.data(), path.length()); 
    cb = _cb;
    return RecursiveIterateDir(depth);
}





constexpr char windowsFilenameRegexStr[] = R"_(^(?!(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][1-9]|[lL][pP][tT][1-9])$)[^<>:"\/\\|?*\x00-\x1F]*[^<>:"\/\\|?*\x00-\x1F .]$)_";
constexpr char windowsInvalidPathRegexStr[] = R"_(([cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][1-9]|[lL][pP][tT][1-9])([\\].*|$))_";
constexpr char windowsPathRegexStr[] = R"_(^([a-zA-Z]:\\)(([^<>:"\/\\|?*\x00-\x1F]*[^<>:"\/\\|?*\x00-\x1F. ])(?:\\)*)*$)_";

bool DirUtil::IsValidFilename(const char* filenameStr, int32_t length) {
    auto matchResult = ctre::match<windowsFilenameRegexStr>(filenameStr, filenameStr + length);
    if (matchResult) {
        return true;
    }
    return false;
}


bool DirUtil::IsValidPath(const char* pathStr, int32_t length) {
    auto invalidMatchResult = ctre::search<windowsInvalidPathRegexStr>(pathStr, pathStr + length);
    if (invalidMatchResult) {
        return false;
    }
    auto matchResult = ctre::match<windowsPathRegexStr>(pathStr, pathStr + length);
    if (matchResult) {
        return true;
    }
    return false;
}