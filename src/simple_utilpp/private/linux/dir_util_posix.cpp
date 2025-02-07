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
#include "logger_header.h"
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
bool InternalCreateDir(char* path, mode_t mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true; // Directory already exists
        }
        else {
            errno = ENOTDIR; // Not a directory
            return false;
        }
    }

    // If the error is not "no such file or directory", propagate the error
    if (errno != ENOENT) {
        return false;
    }

    char* last_slash = strrchr(path, static_cast<char>(std::filesystem::path::preferred_separator));
    if (last_slash == NULL) {
        return false;
    }

    *last_slash = '\0';
    bool res = InternalCreateDir(path, mode);
    *last_slash = static_cast<char>(std::filesystem::path::preferred_separator);
    if (!res) {
        return false;
    }
    // Create the directory
    if (mkdir(path, mode) != 0) {
        if (errno != EEXIST) {
            return false;
        }
    }
    return true;
}


bool DirUtil::IsExist(std::u8string_view  path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    return err == 0;
}

bool DirUtil::IsDirectory(std::u8string_view path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    if (err == -1)
        return false;

    return S_ISDIR(s.st_mode);
}

bool DirUtil::IsRegular(std::u8string_view  path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    if (err == -1)
        return false;

    return S_ISREG(s.st_mode);
}

bool DirUtil::CreateDir(std::u8string_view  path)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    char* path = PathBuf.GetBufInternal();
    return InternalCreateDir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

F_HANDLE DirUtil::RecursiveCreateFile(std::u8string_view  path, uint32_t flag) {
    PathBuf.SetNormalizePath(path.data(), path.length());
    char* path = PathBuf.GetBufInternal();

    char* last_slash = strrchr(path, static_cast<char>(std::filesystem::path::preferred_separator));
    if (last_slash == NULL) {
        return -1;
    }
    last_slash = '\0';
    bool res = InternalCreateDir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    last_slash = static_cast<char>(std::filesystem::path::preferred_separator);
    if (!res) {
        return -1;
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    return open(path, flag, mode);
}

std::u8string_view DirUtil::AbsolutePath(std::u8string_view  path)
{
    return "";
}

bool DirUtil::Delete(std::u8string_view  path)
{
    return false;
}

uint64_t DirUtil::FileSize(std::u8string_view  path)
{
    struct stat s;
    int err = stat((const char*)path.data(), &s);
    if (err == -1)
        return 0;

    return s.st_size;
}
bool RecursiveIterateDir() {
    char* path = PathBuf.GetBufInternal();
    DIR* dir = opendir(path);
    if (dir == NULL) {
        //SIMPLELOG_LOGGER_DEBUG(nullptr, "opendir  failed ");
        return false;
    }

    dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        PathBuf.AppendPath(ent->d_name, GetStringLength(ent->d_name));
        out.Name = (char8_t*)PathBuf.GetBufInternal();
        if (IsDirectory(ent->d_name)) {
            //SIMPLELOG_LOGGER_DEBUG(nullptr,"iter dir {} ", ent->d_name);
            out.bDir = true;
            cb(out);
            RecursiveIterateDir();
        }
        else if (IsRegular(ent->d_name)) {
            //SIMPLELOG_LOGGER_DEBUG(nullptr,"iter file {} ", ent->d_name);
            out.Size = FileSize(ent->d_name);
            out.bDir = false;
            cb(out);
        }
        PathBuf.PopPath();
    }
    closedir(dir);
    return true;
}
bool DirUtil::IterateDir(std::u8string_view  path, IterateDirCallback _cb)
{
    cb = _cb;
    PathBuf.SetNormalizePath(path.data(), path.length());
    return RecursiveIterateDir();
}
