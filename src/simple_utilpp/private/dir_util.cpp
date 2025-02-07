/**
 *  dir_util.cpp
 * 
 * https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
 * These are the directory management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CreateDirectoryW, CreateDirectoryExW GetCurrentDirectoryW RemoveDirectoryW SetCurrentDirectoryW.
 * These are the file management functions that no longer have MAX_PATH restrictions if you opt-in to long path behavior: CopyFileW, CopyFile2, CopyFileExW, CreateFileW, CreateFile2, CreateHardLinkW, CreateSymbolicLinkW, DeleteFileW, FindFirstFileW, FindFirstFileExW, FindNextFileW, GetFileAttributesW, GetFileAttributesExW, SetFileAttributesW, GetFullPathNameW, GetLongPathNameW, MoveFileW, MoveFileExW, MoveFileWithProgressW, ReplaceFileW, SearchPathW, FindFirstFileNameW, FindNextFileNameW, FindFirstStreamW, FindNextStreamW, GetCompressedFileSizeW, GetFinalPathNameByHandleW.
 */

#include "dir_util.h"
#include "dir_util_internal.h"
#include <filesystem>
thread_local FPathBuf PathBuf;
thread_local FPathBuf PathBuf2;


std::u8string_view DirUtil::Normalize(std::u8string_view  path)
{
    PathBuf.SetNormalizePath(path.data(), path.length());
    return (const char8_t*)PathBuf.GetBuf();
}

std::u8string_view DirUtil::FileName(std::u8string_view  path)
{
    std::filesystem::path p(path);
    auto filename=p.filename();
    PathBuf.SetPath((char*)filename.u8string().c_str(), filename.u8string().size() + 1);
    return(const char8_t*)PathBuf.GetBuf();
}

//bool DirUtil::IterateDir(std::u8string_view  path, IterateDirCallback cb)
//{
//    DirEntry_t out;
//    for (auto const& dir_entry : std::filesystem::directory_iterator{ path })
//    {
//        out.Size = dir_entry.file_size();
//        out.Name = dir_entry.path().filename().u8string().c_str();
//        if (dir_entry.is_directory()) {
//            out.bDir = true;
//        }
//        else {
//            out.bDir = false;
//        }
//        cb(out);
//    }
//    return true;
//}