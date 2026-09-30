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
#include "simple_error.h"
#include "RawFile.h"
#include "FunctionExitHelper.h"
#include <wchar.h>
#include <ctre-unicode.hpp>
#include <simple_os_defs.h>

 //https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid
#include <shlobj_core.h>
#include <shobjidl.h>   // IShellLink, IPersistFile
#include <shlguid.h>    // IID_IShellLink, IID_IPersistFile
#include <intshcut.h>      // FMTID_Intshcut, PID_IS_ICONFILE, PID_IS_ICONINDEX 等
#include <objbase.h>       // IPropertySetStorage, IPropertyStorage
#pragma comment(lib, "propsys.lib")
thread_local DirEntry_t out;
bool InternalCreateDir(wchar_t* pathw, size_t prependlen, size_t len) {
    for (size_t i = prependlen; i <= len; i++) {
        if (!(pathw[i] == static_cast<wchar_t>(std::filesystem::path::preferred_separator) || i == len)) {
            continue;
        }
        pathw[i] = L'\0';
        FunctionExitHelper_t restoreSepHelper(
            [&]() {
                pathw[i] = static_cast<wchar_t>(std::filesystem::path::preferred_separator);
            }
        );
        DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
        bool ok = (attr != INVALID_FILE_ATTRIBUTES)
            ? (attr & FILE_ATTRIBUTE_DIRECTORY)
            : CreateDirectoryW(pathw, nullptr);

        if (!ok) return false;
    }
    return true;
}



bool RecursiveIterateDir(FPathBuf& pathBuf, DirUtil::IterateDirRecursivelyCallback& cb, uint32_t depth = std::numeric_limits<uint32_t>::max(), EIterateDirOrder IterateDirOrder = EIterateDirOrder::IDO_NLR) {
    WIN32_FIND_DATAW wfd;
    thread_local bool bExit{ false };
    thread_local bool bEnter{ true };
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
    //out.Name = (char8_t*)pathBuf.GetBuf();
    out.pPathBuf = &pathBuf;
    do {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
            (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0)) {
            continue;
        }
        pathBuf.AppendPathW(wfd.cFileName, GetStringLengthW(wfd.cFileName));
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (IterateDirOrder == EIterateDirOrder::IDO_NLR) {
                out.bDir = true;
                bEnter = true;
                cb(out, bExit, bEnter);
                if (bExit) {
                    return false;
                }
            }
            if (depth > out.Depth && bEnter) {
                out.Depth++;
                FunctionExitHelper_t depthHelper(
                    [&]() {
                        out.Depth--;
                    }
                );
                if (!RecursiveIterateDir(pathBuf, cb, depth - 1, IterateDirOrder)) {
                    return false;
                }
            }
            if (IterateDirOrder == EIterateDirOrder::IDO_LRN) {
                out.bDir = true;
                cb(out, bExit, bEnter);
                if (bExit) {
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
            cb(out, bExit, bEnter);
        }
        pathBuf.PopPathW();
    } while (FindNextFileW(hf, &wfd) != 0);
    return true;
}
bool RecursiveIterateDir(FPathBuf& pathBuf, DirUtil::IterateDirCallback& cb, uint32_t depth = std::numeric_limits<uint32_t>::max(), EIterateDirOrder IterateDirOrder = EIterateDirOrder::IDO_NLR) {
    DirUtil::IterateDirRecursivelyCallback internalCB = [&](DirEntry_t& e, bool& bExit, bool& bEnter) {
        bExit = !cb(e);
        };
    return RecursiveIterateDir(pathBuf, internalCB, depth, IterateDirOrder);
}

std::u8string_view DirUtil::AbsolutePath(std::u8string_view  path)
{
    PathBuf2.SetPath((char*)path.data(), path.size());
    auto pathw = PathBuf2.GetBufW();
    auto length = GetFullPathNameW((LPCWSTR)pathw, (DWORD)PATH_MAX, PathBuf.GetBufInternalW(), NULL);
    PathBuf.UpdatePathLenW(length);
    return (const char8_t*)PathBuf.GetBuf();
}

bool DirUtil::AbsolutePath(std::u8string_view  path, FPathBuf& pathBuf)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    auto length = GetFullPathNameW((LPCWSTR)PathBuf.GetBufW(), (DWORD)PATH_MAX, pathBuf.GetBufInternalW(), NULL);
    if (length == 0) {
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
    auto pathw = (wchar_t*)pathBuf.GetPrependFileNamespacesW();
    return InternalCreateDir(pathw, pathBuf.PathPrependLen, pathBuf.GetPathLenW());
}

F_HANDLE DirUtil::RecursiveCreateFile(std::u8string_view  path, uint32_t flag) {
    PathBuf.SetNormalizePathW(path.data(), path.length());
    return RecursiveCreateFile(PathBuf, flag);
}

F_HANDLE DirUtil::RecursiveCreateFile(FPathBuf& pathBuf, uint32_t flag)
{
    auto pathw = (wchar_t*)pathBuf.GetPrependFileNamespacesW();
    auto rawpathw = (wchar_t*)pathBuf.GetBufInternalW();

    DWORD attr = GetFileAttributesW((LPCWSTR)pathw);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            return INVALID_HANDLE_VALUE;
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
    for (size_t i = 0; i < pathBuf.GetPathLenW(); i++) {
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

F_HANDLE DirUtil::RecursiveCreateFile(FPathBuf& pathBuf, uint32_t flag, std::error_code& ec)
{

    DWORD attr = GetFileAttributesW((LPCWSTR)pathBuf.GetPrependFileNamespacesW());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            ec = std::make_error_code(std::errc::permission_denied);
            return INVALID_HANDLE_VALUE;
        }
        else {
            ec = std::make_error_code(std::errc::file_exists);
        }
    }

    F_HANDLE handle = CreateFileW((LPCWSTR)pathBuf.GetPrependFileNamespacesW(),
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
    if (flag == UTIL_CREATE_ALWAYS || flag == UTIL_OPEN_ALWAYS) {
        if (!(err == ERROR_PATH_NOT_FOUND || err == ERROR_FILE_NOT_FOUND)) {
            ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
            SIMPLELOG_LOGGER_WARN(nullptr, "Can't open the file : {}. ErrorCode is {}", pathw, err);
            return handle;
        }
    }
    else {
        if (err == ERROR_PATH_NOT_FOUND || err == ERROR_FILE_NOT_FOUND) {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return handle;
        }
        else {
            ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
            SIMPLELOG_LOGGER_WARN(nullptr, "Can't open the file : {}. ErrorCode is {}", pathw, err);
            return handle;
        }
    }
    auto fileName = pathBuf.PopPathW();
    // Path not found? Create the directory
    if (!InternalCreateDir((wchar_t*)pathBuf.GetPrependFileNamespacesW(), pathBuf.PathPrependLen, pathBuf.GetPathLenW() + pathBuf.PathPrependLen)) {
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
        return handle;
    }
    pathBuf.AppendPathW(ConvertU16ViewToWView(fileName));
    handle = CreateFileW((LPCWSTR)pathBuf.GetPrependFileNamespacesW(),
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        flag,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't open the file after create directory: {}. ErrorCode is {}", pathw, err);
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_UNKNOW);
    }
    ec.clear();
    return handle;
}

bool DirUtil::Delete(std::u8string_view  path)
{
    PathBuf.SetPath((char*)path.data(), path.size());
    return Delete(PathBuf);
}

bool DirUtil::Delete(FPathBuf& pathBuf)
{
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
        auto bres = RecursiveIterateDir(pathBuf, cb, std::numeric_limits<uint32_t>::max(), EIterateDirOrder::IDO_LRN);
        if (bres) {
            bres = RemoveDirectoryW((LPCWSTR)pathw) == TRUE;
        }
        return bres;
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
    auto path_oldw = PathBuf.GetPrependFileNamespacesW();
    auto pathw = PathBuf2.GetPrependFileNamespacesW();
    auto bres = MoveFileExW(path_oldw, pathw, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == TRUE;
    if (!bres) {
        auto winerr = GetLastError();
        if (winerr == ERROR_PATH_NOT_FOUND) {
            auto bDir = IsDirectory(PathBuf);
            if (bDir) {
                CreateDir(PathBuf2);
            }
            else {
                auto fileName = PathBuf2.PopPathW();
                CreateDir(PathBuf2);
                if (fileName.size()) {
                    PathBuf2.AppendPathW(ConvertU16ViewToWView(fileName));
                }
            }
            bres = MoveFileExW(path_oldw, pathw, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == TRUE;
        }
    }
    return bres;
}

bool DirUtil::Copy(std::u8string_view  oldpath, std::u8string_view  path, CopyProgressCallback cb) {
    PathBuf.SetPath((char*)oldpath.data(), oldpath.size());
    PathBuf2.SetPath((char*)path.data(), path.size());
    return Copy(PathBuf, PathBuf2, cb);
}
bool DirUtil::Copy(FPathBuf& pathBuf, FPathBuf& newfilePathBuf, CopyProgressCallback cb) {
    if (!IsExist(pathBuf)) {
        return false;
    }
    bool bCopyFile{ true };
    if (IsDirectory(pathBuf)) {
        bCopyFile = false;
    }
    if (IsExist(newfilePathBuf)) {
        if (!bCopyFile && !IsDirectory(newfilePathBuf)) {
            return false;
        }
    }
    else {
        if (bCopyFile) {
            auto filename = newfilePathBuf.PopPathW();
            if (!CreateDir(newfilePathBuf)) {
                return false;
            }
            newfilePathBuf.AppendPathW((const wchar_t*)filename.data(), filename.size());
        }
        else {
            if (!CreateDir(newfilePathBuf)) {
                return false;
            }
        }
    }
    if (bCopyFile) {
        return CopyFileW(pathBuf.GetBufW(), newfilePathBuf.GetBufW(), false);
    }
    else {
        constexpr int bufLen = 1 << 12;
        char buf[bufLen];
        FRawFile oldFile;
        FRawFile newFile;
        bool bres;
        int64_t writed;
        int16_t needread;
        IterateDir(pathBuf,
            [&](DirEntry_t& entry)->bool {
                auto relatePath = entry.pPathBuf->PopPathW(entry.Depth + 1, false);
                newfilePathBuf.AppendPathW(ConvertU16ViewToWView(relatePath));
                FunctionExitHelper_t newfilePathBufGuard(
                    [&]() {
                        newfilePathBuf.PopPathW(entry.Depth + 1);
                    }
                );
                if (entry.bDir) {
                    CreateDir(newfilePathBuf);
                }
                else {
                    if (cb) {
                        if (!cb(entry, 0)) {
                            return true;
                        }
                    }
                    if (oldFile.Open(*entry.pPathBuf, UTIL_OPEN_EXISTING) != ERR_SUCCESS) {
                        bres = false;
                        return false;
                    }
                    auto fileSize = oldFile.GetSize();
                    if (newFile.Open(newfilePathBuf, UTIL_CREATE_ALWAYS, fileSize) != ERR_SUCCESS) {
                        bres = false;
                        return false;
                    }

                    for (writed = 0; writed < fileSize;) {
                        needread = fileSize - writed > bufLen ? bufLen : fileSize - writed;
                        if (oldFile.Read(buf, needread) != ERR_SUCCESS) {
                            bres = false;
                            return false;
                        }
                        if (newFile.Write(buf, needread) != ERR_SUCCESS) {
                            bres = false;
                            return false;
                        }
                        writed += needread;
                        if (cb) {
                            if (!cb(entry, writed)) {
                                newFile.Close();
                                DirUtil::Delete(newfilePathBuf);
                                return true;
                            }
                        }
                    }
                }
                return true;
            }
        );
    }
    return true;
}

bool DirUtil::IterateDir(std::u8string_view  path, IterateDirCallback _cb, uint32_t depth, EIterateDirOrder IterateDirOrder)
{
    PathBuf.SetNormalizePathW(path.data(), path.length());
    return IterateDir(PathBuf, _cb, depth, IterateDirOrder);
}

bool DirUtil::IterateDir(FPathBuf& pathBuf, IterateDirCallback cb, uint32_t depth, EIterateDirOrder IterateDirOrder)
{
    return RecursiveIterateDir(pathBuf, cb, depth, IterateDirOrder);
}

bool DirUtil::IterateDirRecursively(std::u8string_view path, IterateDirRecursivelyCallback cb)
{
    PathBuf.SetNormalizePathW(path.data(), path.length());
    return IterateDirRecursively(PathBuf, cb);
}

bool DirUtil::IterateDirRecursively(FPathBuf& pathBuf, IterateDirRecursivelyCallback cb)
{
    return RecursiveIterateDir(pathBuf, cb, std::numeric_limits<uint32_t>::max(), EIterateDirOrder::IDO_NLR);
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

std::u8string_view DirUtil::SearchFileInPath(std::u8string_view fileName, std::error_code& ec)
{
    PathBuf.SetPath((char*)fileName.data(), fileName.size());
    return SearchFileInPath(PathBuf, ec);
}

std::u8string_view DirUtil::SearchFileInPath(FPathBuf& pathBuf, std::error_code& ec)
{
    auto pathw = (wchar_t*)pathBuf.GetPrependFileNamespacesW();
    LPWSTR lastCursor = nullptr;
    auto len = SearchPathW(NULL, pathw, NULL, PATH_MAX, PathBuf2.GetBufInternalW(), &lastCursor);
    if (len == 0) {
        ec = std::error_code(GetLastError(), std::system_category());
        return std::u8string_view();
    }
    ec.clear();
    return PathBuf2.GetU8View();
}

std::u8string_view DirUtil::GetVSwherePath(std::error_code& ec)
{
    PWSTR programFilesX86Path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, NULL, &programFilesX86Path);
    FunctionExitHelper_t guardPath(
        [&programFilesX86Path]() {
            CoTaskMemFree(programFilesX86Path);
        }
    );
    if (FAILED(hr)) {
        ec = utilpp::make_common_used_error(utilpp::ECommonUsedError::CUE_NOT_SUPPORT);
        return std::u8string_view();
    }
    PathBuf.SetPathW(programFilesX86Path, GetStringLengthW(programFilesX86Path));
    PathBuf.AppendPathW(L"Microsoft Visual Studio");
    PathBuf.AppendPathW(L"Installer");
    PathBuf.AppendPathW(L"vswhere.exe");
    return PathBuf.GetU8View();
}

std::u8string_view DirUtil::GetOSDirectory(std::error_code& ec)
{
    auto len = GetSystemDirectoryA(PathBuf.GetBufInternal(), PATH_MAX);
    if (len == 0) {
        ec = std::error_code(GetLastError(), std::system_category());
        return std::u8string_view();
    }
    PathBuf.UpdatePathLen(len);
    ec.clear();
    return PathBuf.GetU8View();
}

std::u8string_view DirUtil::GetDesktopPath(std::error_code& ec)
{
    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &path);
    FunctionExitHelper_t guardPath(
        [&path]() {
            CoTaskMemFree(path);
        }
    );
    if (FAILED(hr)) {
        ec = std::error_code(GetLastError(), std::system_category());
        return std::u8string_view();
    }
    PathBuf.SetPathW(path, GetStringLengthW(path));
    return PathBuf.GetU8View();
}

bool DirUtil::CreateShortcut(ShortcutOptions_t ShortcutOptions)
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        return false;
    }
    IPersistFile* pPersistFile = nullptr;
    bool success = false;
    std::filesystem::path tagetPath = ShortcutOptions.ShortcutPath;
    if (tagetPath.extension() == ".lnk") {
        IShellLinkW* pShellLink = nullptr;
        auto TargetPath16 = U8ToU16(ShortcutOptions.TargetPath);
        do {
            // 1. 创建 IShellLink 实例
            hr = CoCreateInstance(
                CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                IID_IShellLinkW, reinterpret_cast<void**>(&pShellLink));
            if (FAILED(hr)) break;

            // 2. 设置目标路径
            hr = pShellLink->SetPath(ConvertU16ViewToWView(TargetPath16).data());
            if (FAILED(hr)) break;

            // 3. 设置工作目录
            if (!ShortcutOptions.WorkDir.empty()) {
                auto WorkDir16 = U8ToU16(ShortcutOptions.WorkDir);
                pShellLink->SetWorkingDirectory(ConvertU16ViewToWView(WorkDir16).data());
                if (FAILED(hr)) break;
            }
            if (!ShortcutOptions.IconPath.empty()) {
                auto IconPath16 = U8ToU16(ShortcutOptions.IconPath);
                hr = pShellLink->SetIconLocation(ConvertU16ViewToWView(IconPath16).data(), 0);
                if (FAILED(hr)) break;
            }

            // 5. 获取 IPersistFile 接口以保存到磁盘
            hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
            if (FAILED(hr)) break;

            // 6. 保存 .lnk 文件
            auto ShortcutPath16 = U8ToU16(ShortcutOptions.ShortcutPath);
            hr = pPersistFile->Save(ConvertU16ViewToWView(ShortcutPath16).data(), TRUE);
            if (SUCCEEDED(hr)) success = true;

        } while (false);
        if (pPersistFile) pPersistFile->Release();
        if (pShellLink)   pShellLink->Release();
    }
    else {
        //https://learn.microsoft.com/en-us/windows/win32/lwef/internet-shortcuts#creating-an-internet-shortcut-from-a-url
        IUniformResourceLocatorW* pUrl = nullptr;
        IPropertySetStorage* pPropSetStg = nullptr;
        IPropertyStorage* pPropStg = nullptr;
        auto TargetPath16 = U8ToU16(ShortcutOptions.TargetPath);
        auto ShortcutPath16 = U8ToU16(ShortcutOptions.ShortcutPath);
        do {
            hr = CoCreateInstance(
                CLSID_InternetShortcut, nullptr, CLSCTX_INPROC_SERVER,
                IID_IUniformResourceLocatorW,
                reinterpret_cast<void**>(&pUrl));
            if (FAILED(hr)) break;

            hr = pUrl->SetURL(ConvertU16ViewToWView(TargetPath16).data(), 0);
            if (FAILED(hr)) break;

            hr = pUrl->QueryInterface(IID_IPersistFile,
                reinterpret_cast<void**>(&pPersistFile));
            if (FAILED(hr)) break;

            if (!ShortcutOptions.IconPath.empty()) {
                auto IconPath16 = U8ToU16(ShortcutOptions.IconPath);

                hr = pPersistFile->Save(ConvertU16ViewToWView(ShortcutPath16).data(), TRUE);
                if (FAILED(hr)) break;

                hr = pUrl->QueryInterface(IID_IPropertySetStorage,
                    reinterpret_cast<void**>(&pPropSetStg));
                if (FAILED(hr)) break;

                hr = pPropSetStg->Open(
                    FMTID_Intshcut,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                    &pPropStg);
                if (FAILED(hr)) break;

                // 5. 写入图标属性
                PROPSPEC propspec[2] = {};
                PROPVARIANT propvar[2] = {};

                // IconFile
                propspec[0].ulKind = PRSPEC_PROPID;
                propspec[0].propid = PID_IS_ICONFILE;
                propvar[0].vt = VT_LPWSTR;
                propvar[0].pwszVal = (LPWSTR)ConvertU16ViewToWView(IconPath16).data();

                // IconIndex
                propspec[1].ulKind = PRSPEC_PROPID;
                propspec[1].propid = PID_IS_ICONINDEX;
                propvar[1].vt = VT_I4;
                propvar[1].lVal = 0;

                hr = pPropStg->WriteMultiple(2, propspec, propvar, 0);
                if (FAILED(hr)) break;

                // 6. 提交并重新保存
                hr = pPropStg->Commit(STGC_DEFAULT);
                if (FAILED(hr)) break;
            }

            hr = pPersistFile->Save(ConvertU16ViewToWView(ShortcutPath16).data(), TRUE);
            success = SUCCEEDED(hr);

        } while (false);

        if (pPropStg)     pPropStg->Release();
        if (pPropSetStg)  pPropSetStg->Release();
        if (pPersistFile) pPersistFile->Release();
        if (pUrl)         pUrl->Release();
    }

    CoUninitialize();
    return success;
}