/**
 *  dir_util.h
 *      utility for directory operation
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>


struct DirEntry
{
    std::string name;
    uint64_t size;
    bool dir;
};

class DirUtil
{
public:
    DirUtil();
    ~DirUtil();

public:
    static std::string Normalize(std::string path);
    static std::string UncHelper(std::string path);
    static bool IsExist(std::string path);
    static bool IsDirectory(std::string path);
    static bool IsRegular(std::string path);
    static uint64_t FileSize(std::string path);
    static bool SetWritable(std::string path);

    static bool CreateDir(std::string path);
    static std::string AbsolutePath(std::string path);
    static std::string BasePath(std::string path);
    static std::string FileName(std::string path);
    static bool Delete(std::string path);
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
    bool IterateDir(std::string path);
    size_t EntryCount();
    DirEntry GetEntry(size_t index);

private:
    void ClearDir();

private:
    std::vector<DirEntry> entries_;
};