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
#include "FunctionExitHelper.h"
#include <wchar.h>
#include <simple_os_defs.h>
#include <ctre-unicode.hpp>


thread_local DirEntry_t out;
bool InternalCreateDir(wchar_t* pathw, size_t prependlen, size_t len) {
    for (size_t i = prependlen; i < len; i++) {
        if (pathw[i] != static_cast<wchar_t>(std::filesystem::path::preferred_separator)) {
            continue;
        }
        pathw[i] = L'\0';
        FunctionExitHelper_t restoreSepHelper(
            [&]() {
                pathw[i] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
            }
        );
        DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
        if (attr != INVALID_FILE_ATTRIBUTES) {
            if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                return false;
            }
        }
        else {
            if (CreateDirectoryW((LPCWSTR)pathw, NULL) == FALSE) {
                return false;
            }
        }
    }
    return true;
}

bool RecursiveIterateDir(FPathBuf& pathBuf, DirUtil::IterateDirCallback& cb, uint32_t depth = std::numeric_limits<uint32_t>::max(), EIterateDirOrder IterateDirOrder = EIterateDirOrder::IDO_NLR) {
    WIN32_FIND_DATAW wfd;
    bool bContinue{ true };
    pathBuf.AppendPathW(L"*", 1);
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    HANDLE hf = FindFirstFileW(pathw, &wfd);
    if (INVALID_HANDLE_VALUE == hf) {
        return false;
    }
    FunctionExitHelper_t findCloseHelper(
        [&]() {
            FindClose(hf);
        }
    );
    pathBuf.PopPathW();
    out.Name = (char8_t*)pathBuf.GetBufInternal();
    out.pPathBuf = &pathBuf;
    do {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
            (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0)) {
            continue;
        }
        pathBuf.AppendPathW(wfd.cFileName, GetStringLengthW(wfd.cFileName));
        pathBuf.ToPath();
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (IterateDirOrder == EIterateDirOrder::IDO_NLR) {
                out.bDir = true;
                bContinue = cb(out);
                if (!bContinue) {
                    return false;
                }
            }
            if (depth > out.Depth) {
                out.Depth++;
                FunctionExitHelper_t depthHelper(
                    [&]() {
                        out.Depth--;
                    }
                );
                if (!RecursiveIterateDir(pathBuf, cb, depth - 1)) {
                    return false;
                }
            }
            if (IterateDirOrder == EIterateDirOrder::IDO_LRN) {
                out.bDir = true;
                bContinue = cb(out);
                if (!bContinue) {
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
        pathBuf.PopPathW();
    } while (FindNextFileW(hf, &wfd) != 0);
    return true;
}

std::u8string_view DirUtil::AbsolutePath(std::u8string_view  path)
{
    PathBuf2.SetPath((char*)path.data(), path.size());
    PathBuf2.ToPathW();
    auto pathw = PathBuf2.GetBufW();
    auto length = GetFullPathNameW((LPCWSTR)pathw, (DWORD)PATH_MAX, PathBuf.GetBufInternalW(), NULL);
    PathBuf.UpdatePathLenW(length);
    PathBuf.ToPath();
    return (const char8_t*)PathBuf.GetBuf();
}

bool DirUtil::AbsolutePath(std::u8string_view  path, FPathBuf& pathBuf)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    PathBuf.ToPathW();
    auto length = GetFullPathNameW((LPCWSTR)PathBuf.GetBufW(), (DWORD)PATH_MAX, pathBuf.GetBufInternalW(), NULL);
    if (length == 0){
        return false;
    }
    pathBuf.UpdatePathLenW(length);
    return true;
}

bool DirUtil::SetWritable(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return SetWritable(PathBuf);
}
bool DirUtil::SetWritable(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_READONLY) {
        attr &= ~FILE_ATTRIBUTE_READONLY;
        if (!SetFileAttributesW((LPCWSTR)pathw, attr)) {
            return false;
        }
    }
    return true;
}
bool DirUtil::IsExist(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return IsExist(PathBuf);
}

bool DirUtil::IsExist(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return (attr != INVALID_FILE_ATTRIBUTES);
}

bool DirUtil::IsDirectory(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return IsDirectory(PathBuf);
}

bool DirUtil::IsDirectory(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirUtil::IsRegular(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return IsRegular(PathBuf);
}

bool DirUtil::IsRegular(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirUtil::SetCWD(std::u8string_view path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return SetCWD(PathBuf);
}

bool DirUtil::SetCWD(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    return SetCurrentDirectoryW(pathw);
}

uint64_t DirUtil::FileSize(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return FileSize(PathBuf);
}

uint64_t DirUtil::FileSize(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
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
    PathBuf.SetPath((char*)path.data(), path.size());
    return CreateDir(PathBuf);
}

bool DirUtil::CreateDir(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = (wchar_t*)pathBuf.GetPrependFileNamespacesW();
    return InternalCreateDir(pathw, pathBuf.PathPrependLen, pathBuf.PathLenW);
}

F_HANDLE DirUtil::RecursiveCreateFile(std::u8string_view  path, uint32_t flag) {
    PathBuf.SetNormalizePathW(path.data(), path.length());
    return RecursiveCreateFile(PathBuf, flag);
}

F_HANDLE DirUtil::RecursiveCreateFile(FPathBuf& pathBuf, uint32_t flag)
{
    pathBuf.ToPathW();
    auto pathw = (wchar_t*)pathBuf.GetPrependFileNamespacesW();
    auto rawpathw = (wchar_t*)pathBuf.GetBufInternalW();

    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            return NULL;
        }
    }

    F_HANDLE handle = CreateFileW((LPCWSTR)pathw,
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
    wchar_t* lastCursor = wcsrchr(rawpathw, static_cast<wchar_t>(std::filesystem::path::preferred_separator));
    if (lastCursor == NULL) {
        return handle;
    }
    if (!InternalCreateDir(pathw, pathBuf.PathPrependLen, lastCursor - rawpathw)) {
        return handle;
    }
    for (size_t i = 0; i < pathBuf.PathLenW; i++) {
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
    return Delete(PathBuf);
}

bool DirUtil::Delete(FPathBuf& pathBuf)
{
    pathBuf.ToPathW();
    auto pathw = pathBuf.GetPrependFileNamespacesW();
    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        std::function<bool(DirEntry_t&)> cb = [](DirEntry_t& entry)->bool {
            auto pathw = entry.pPathBuf->GetPrependFileNamespacesW();
            if (!entry.bDir) {
                return DeleteFileW((LPCWSTR)pathw) == TRUE;
            }
            else {
                return RemoveDirectoryW((LPCWSTR)pathw) == TRUE;
            }
            };
        return RecursiveIterateDir(pathBuf, cb, std::numeric_limits<uint32_t>::max(), EIterateDirOrder::IDO_LRN);
    }
    else {
        return DeleteFileW((LPCWSTR)pathw) == TRUE;
    }
}

bool DirUtil::Rename(std::u8string_view oldpath, std::u8string_view path)
{
    PathBuf.SetPath((char*)oldpath.data(), oldpath.size());
    PathBuf2.SetPath((char*)path.data(), path.size());
    return Rename(PathBuf, PathBuf2);
}

bool DirUtil::Rename(FPathBuf& pathBuf, FPathBuf& newfilePathBuf)
{
    pathBuf.ToPathW();
    newfilePathBuf.ToPathW();
    auto path_oldw = PathBuf.GetPrependFileNamespacesW();
    auto pathw = PathBuf2.GetPrependFileNamespacesW();
    return MoveFileExW(path_oldw, pathw, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == TRUE;
}

bool DirUtil::IterateDir(std::u8string_view  path, IterateDirCallback _cb, uint32_t depth, EIterateDirOrder IterateDirOrder)
{
    PathBuf.SetNormalizePathW(path.data(), path.length());
    return IterateDir(PathBuf, _cb, depth, IterateDirOrder);
}

bool DirUtil::IterateDir(FPathBuf& pathBuf, IterateDirCallback cb, uint32_t depth, EIterateDirOrder IterateDirOrder)
{
    pathBuf.ToPathW();
    return RecursiveIterateDir(pathBuf, cb, depth, IterateDirOrder);
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