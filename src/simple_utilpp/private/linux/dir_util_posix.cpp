/**
 *  dir_util.cpp
 *
 * https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
 * These are the directory management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CreateDirectoryW, CreateDirectoryExW GetCurrentDirectoryW RemoveDirectoryW SetCurrentDirectoryW.
 * These are the file management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CopyFileW, CopyFile2, CopyFileExW, CreateFileW, CreateFile2, CreateHardLinkW, CreateSymbolicLinkW, DeleteFileW, FindFirstFileW, FindFirstFileExW, FindNextFileW, GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW, GetFullPathNameW, GetLongPathNameW, MoveFileW, MoveFileExW, MoveFileWithProgressW, ReplaceFileW, SearchPathW, FindFirstFileNameW, FindNextFileNameW, FindFirstStreamW, FindNextStreamW, GetCompressedFileSizeW, GetFinalPathNameByHandleW.
 */

#include "dir_util.h"
#include "dir_util_internal.h"
#include "string_convert.h"

#include <stack>
#include <assert.h>
#include <filesystem>
#include <simple_os_defs.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
thread_local DirUtil::IterateDirCallback cb;
thread_local DirEntry_t out;
bool InternalCreateDir(char *path, mode_t mode)
{
    struct stat st;
    if (stat(path, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return true; // Directory already exists
        }
        else
        {
            errno = ENOTDIR; // Not a directory
            return false;
        }
    }

    // If the error is not "no such file or directory", propagate the error
    if (errno != ENOENT)
    {
        return false;
    }

    char *last_slash = strrchr(path, static_cast<char>(std::filesystem::path::preferred_separator));
    if (last_slash == NULL)
    {
        return false;
    }

    *last_slash = '\0';
    bool res = InternalCreateDir(path, mode);
    *last_slash = static_cast<char>(std::filesystem::path::preferred_separator);
    if (!res)
    {
        return false;
    }
    // Create the directory
    if (mkdir(path, mode) != 0)
    {
        if (errno != EEXIST)
        {
            return false;
        }
    }
    return true;
}
std::u8string_view DirUtil::AbsolutePath(std::u8string_view path)
{
    return u8"";
}

bool DirUtil::IsExist(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char *)path.data(), &s);
    return err == 0;
}

bool DirUtil::IsExist(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return IsExist(ConvertViewToU8View(path));
}

bool DirUtil::IsDirectory(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char *)path.data(), &s);
    if (err == -1)
        return false;

    return S_ISDIR(s.st_mode);
}

bool DirUtil::IsDirectory(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return IsDirectory(ConvertViewToU8View(path));
}

bool DirUtil::IsRegular(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char *)path.data(), &s);
    if (err == -1)
        return false;

    return S_ISREG(s.st_mode);
}

bool DirUtil::IsRegular(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return IsRegular(ConvertViewToU8View(path));
}

bool DirUtil::SetCWD(std::u8string_view path)
{
    int err = chdir((const char*)path.data());
    if (err == -1)
        return false;

    return true
}

bool DirUtil::SetCWD(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return SetCWD(ConvertViewToU8View(path));
}

uint64_t DirUtil::FileSize(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    if (err == -1)
        return 0;

    return s.st_size;
}

uint64_t DirUtil::FileSize(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return FileSize(ConvertViewToU8View(path));
}

bool DirUtil::SetWritable(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    if (err == -1)
        return false;
    mode_t new_mode = s.st_mode | S_IWUSR;
    err = chmod((const char*)path.data(), new_mode);
    if (err == -1)
        return false;
    return true;
}

bool DirUtil::SetWritable(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return SetWritable(ConvertViewToU8View(path));
}

bool DirUtil::CreateDir(std::u8string_view path)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    char *norPath = PathBuf.GetBufInternal();
    return InternalCreateDir(norPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool DirUtil::CreateDir(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return CreateDir(ConvertViewToU8View(path));
}

F_HANDLE DirUtil::RecursiveCreateFile(std::u8string_view path, uint32_t flag)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    char *norPath = PathBuf.GetBufInternal();

    char *last_slash = strrchr(norPath, static_cast<char>(std::filesystem::path::preferred_separator));
    if (last_slash == NULL)
    {
        return -1;
    }
    *last_slash = '\0';
    bool res = InternalCreateDir(norPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    *last_slash = static_cast<char>(std::filesystem::path::preferred_separator);
    if (!res)
    {
        return -1;
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    return open(norPath, flag, mode);
}

F_HANDLE DirUtil::RecursiveCreateFile(FPathBuf& pathBuf) {
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return RecursiveCreateFile(ConvertViewToU8View(path));
}

bool DirUtil::Delete(std::u8string_view path)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    char* norPath = PathBuf.GetBufInternal();
    return remove(norPath) == 0;
}

bool DirUtil::Delete(FPathBuf& pathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    return Delete(ConvertViewToU8View(path));
}

bool DirUtil::Rename(std::u8string_view  oldpath, std::u8string_view  path)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    char* OldPathStr = PathBuf.GetBufInternal();
    PathBuf2.SetNormalizePath(path.data(), path.length());
    char* PathStr = PathBuf2.GetBufInternal();
    return rename(OldPathStr, PathStr)==0;
}

bool DirUtil::Rename(FPathBuf& pathBuf, FPathBuf& newPathBuf)
{
    pathBuf.ToPath();
    auto path = pathBuf.GetPrependFileNamespaces();
    newPathBuf.ToPath();
    auto newPath = newPathBuf.GetPrependFileNamespaces();
    return Rename(ConvertViewToU8View(path), ConvertViewToU8View(newPath));
}

bool RecursiveIterateDir(uint32_t depth)
{
    char *path = PathBuf.GetBufInternal();
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        // SIMPLELOG_LOGGER_DEBUG(nullptr, "opendir  failed ");
        return false;
    }

    dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        PathBuf.AppendPath(ent->d_name, GetStringLength(ent->d_name));
        out.Name = (char8_t *)PathBuf.GetBufInternal();
        if (DirUtil::IsDirectory((const char8_t *)ent->d_name))
        {
            // SIMPLELOG_LOGGER_DEBUG(nullptr,"iter dir {} ", ent->d_name);
            out.bDir = true;
            cb(out);
            if (depth > 0)
            {
                RecursiveIterateDir(depth - 1);
            }
        }
        else if (DirUtil::IsRegular((const char8_t *)ent->d_name))
        {
            // SIMPLELOG_LOGGER_DEBUG(nullptr,"iter file {} ", ent->d_name);
            out.Size = DirUtil::FileSize((const char8_t *)ent->d_name);
            out.bDir = false;
            cb(out);
        }
        PathBuf.PopPath();
    }
    closedir(dir);
    return true;
}
bool DirUtil::IterateDir(std::u8string_view path, IterateDirCallback _cb, uint32_t depth)
{
    cb = _cb;
    PathBuf.SetNormalizePath(path.data(), path.length());
    return RecursiveIterateDir(depth);
}




bool IsValidFilename(const char* filenameStr, int32_t length) {
    //todo
    return false;
}


bool IsValidPath(const char* pathStr, int32_t length) {
    //todo
    return false;
}