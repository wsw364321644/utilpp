/**
 *  dir_util.h
 *      utility for directory operation
 */

#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <limits>
#include "simple_os_defs.h"
#include "simple_export_ppdefs.h"

constexpr size_t PATH_MAX = std::numeric_limits<int16_t>::max();
#ifdef _WIN32
constexpr char8_t FILE_NAMESPACES[] = u8"\\\\?\\";
constexpr wchar_t FILE_NAMESPACESW[] = L"\\\\?\\";
constexpr size_t PATH_PREFIX_MAX = std::char_traits<char8_t>::length(FILE_NAMESPACES);
#else
constexpr size_t PATH_PREFIX_MAX = 0;
#endif // _WIN32


typedef struct DirEntry_t
{
    const char8_t* Name;
    uint64_t Size;
    bool bDir;
}DirEntry_t;

class SIMPLE_UTIL_EXPORT DirUtil
{

public:
    static std::u8string_view  Normalize(std::u8string_view path);
    static bool IsExist(std::u8string_view  path);
    static bool IsDirectory(std::u8string_view  path);
    static bool IsRegular(std::u8string_view  path);
    static uint64_t FileSize(std::u8string_view  path);
    static bool SetWritable(std::u8string_view  path);
    static bool CreateDir(std::u8string_view  path);
    static std::u8string_view AbsolutePath(std::u8string_view  path);
    static std::u8string_view FileName(std::u8string_view  path);
    static bool Delete(std::u8string_view  path);
    static bool Rename(std::u8string_view  oldpath, std::u8string_view  path);
    static F_HANDLE RecursiveCreateFile(std::u8string_view  path, uint32_t flag);

    typedef std::function<void(DirEntry_t&)> IterateDirCallback;
    static bool IterateDir(std::u8string_view  path, IterateDirCallback cb, uint32_t depth = std::numeric_limits<uint32_t>::max());


    static bool IsValidFilename(const char* filenameStr, int32_t length);
    static bool IsValidPath(const char* pathStr, int32_t length);
};