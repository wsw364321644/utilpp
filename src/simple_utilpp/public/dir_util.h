/**
 *  dir_util.h
 *      utility for directory operation
 */
#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <functional>
#include <limits>
#include "PathBuf.h"
#include "simple_os_defs.h"
#include "simple_export_ppdefs.h"

typedef struct DirEntry_t
{
    const char8_t* Name{ nullptr };
    uint64_t Size{ 0 };
    bool bDir;
    uint32_t Depth{ 0 };
    FPathBuf* pPathBuf{ nullptr };
}DirEntry_t;

enum class EIterateDirOrder {
    IDO_NLR,
    IDO_LRN,
};

/// <summary>
/// require FPathBuf wchar_t path
///
/// </summary>
class SIMPLE_UTIL_EXPORT DirUtil
{

public:
    static std::u8string_view  Normalize(std::u8string_view path);
    static std::u8string_view AbsolutePath(std::u8string_view  path);
    static bool AbsolutePath(std::u8string_view  path,FPathBuf& pathBuf);
    static std::u8string_view FileName(std::u8string_view  path);
    static bool IsExist(std::u8string_view  path);
    static bool IsExist(FPathBuf& pathBuf);
    static bool IsDirectory(std::u8string_view  path);
    static bool IsDirectory(FPathBuf& pathBuf);
    static bool IsRegular(std::u8string_view  path);
    static bool IsRegular(FPathBuf& pathBuf);
    static bool SetCWD(std::u8string_view  path);
    static bool SetCWD(FPathBuf& pathBuf);
    static uint64_t FileSize(std::u8string_view  path);
    static uint64_t FileSize(FPathBuf& pathBuf);
    static bool SetWritable(std::u8string_view  path);
    static bool SetWritable(FPathBuf& pathBuf);

    /// create directory util last separator 
    /// pass a/b/c will create folder a/b
    /// pass a/b/c/ will create folder a/b/c/
    static bool CreateDir(std::u8string_view  path);
    static bool CreateDir(FPathBuf& pathBuf);
    static bool Delete(std::u8string_view  path);
    static bool Delete(FPathBuf& pathBuf);
    static bool Rename(std::u8string_view  oldpath, std::u8string_view  path);
    static bool Rename(FPathBuf& pathBuf, FPathBuf& newfilePathBuf);
    static F_HANDLE RecursiveCreateFile(std::u8string_view  path, uint32_t flag);
    static F_HANDLE RecursiveCreateFile(FPathBuf& pathBuf, uint32_t flag);

    /// return false will stop iteration
    typedef std::function<bool(DirEntry_t&)> IterateDirCallback;
    static bool IterateDir(std::u8string_view  path, IterateDirCallback cb, uint32_t depth = std::numeric_limits<uint32_t>::max(), EIterateDirOrder IterateDirOrder= EIterateDirOrder::IDO_NLR);
    static bool IterateDir(FPathBuf& pathBuf, IterateDirCallback cb, uint32_t depth = std::numeric_limits<uint32_t>::max(), EIterateDirOrder IterateDirOrder = EIterateDirOrder::IDO_NLR);

    static bool IsValidFilename(const char* filenameStr, int32_t length);
    static bool IsValidPath(const char* pathStr, int32_t length);
};