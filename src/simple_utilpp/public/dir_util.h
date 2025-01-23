/**
 *  dir_util.h
 *      utility for directory operation
 */

#pragma once

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>
#include <ThroughCRTWrapper.h>
#include "simple_export_ppdefs.h"

struct DirEntry
{
    std::string name;
    uint64_t size;
    bool dir;
};

class SIMPLE_UTIL_EXPORT DirUtil
{
public:
    DirUtil();
    ~DirUtil();

public:
    static std::string Normalize(std::u8string_view path);
    static std::string UncHelper(std::u8string_view path);
    static bool IsExist(std::u8string_view  path);
    static bool IsDirectory(std::u8string_view  path);
    static bool IsRegular(std::u8string_view  path);
    static uint64_t FileSize(std::u8string_view  path);
    static bool SetWritable(std::u8string_view  path);

    static bool CreateDir(std::u8string_view  path);
    static std::string AbsolutePath(std::u8string_view  path);
    static std::string BasePath(std::u8string_view  path);
    static std::string FileName(std::u8string_view  path);
    static bool Delete(std::u8string_view  path);
    //static std::wstring Normalize(std::wstring path);
    //static std::wstring UncHelper(std::wstring path);
    //static bool IsExist(std::wstring path);
    //static bool IsDirectory(std::wstring path);
    //static bool IsRegular(std::wstring path);
    //static uint64_t FileSize(std::wstring path);
    //static bool SetWritable(std::wstring path);

    //static bool CreateDir(std::wstring path);
    //static std::wstring AbsolutePath(std::wstring path);
    //static std::wstring BasePath(std::wstring path);
    //static std::wstring FileName(std::wstring path);
    //static bool Delete(std::wstring path);

public:
    //bool IterateDir(std::wstring path);
    bool IterateDir(std::u8string_view  path);
    size_t EntryCount();
    DirEntry GetEntry(size_t index);

private:
    void ClearDir();

private:
    std::vector<DirEntry> entries_;
};