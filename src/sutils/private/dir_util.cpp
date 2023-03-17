/**
 *  dir_util.cpp
 */

#include "dir_util.h"
#include "string_convert.h"
#include "logger.h"
#include <stack>
#include <assert.h>
#include <filesystem>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif

DirUtil::DirUtil()
{}
DirUtil::~DirUtil()
{}
std::string DirUtil::Normalize(std::string path)
{
    std::filesystem::path p((char8_t*)path.c_str());
    return (char*)p.u8string().c_str();
    //auto pos = path.find("/");
    //while (pos != std::wstring::npos) {
    //    path[pos] = '\\';
    //    pos = path.find("/", pos + 1);
    //}
    //return std::move(path);
}


#ifdef WIN32
std::string DirUtil::UncHelper(std::string path)
{
    std::string unc("\\\\?\\");
    // just reserve some space for short name in directory, so minus 64
    if (path.size() > (MAX_PATH - 64) && (memcmp(path.c_str(), unc.c_str(), unc.size()) != 0))
        return unc + path;
    else
        return path;
}

bool DirUtil::SetWritable(std::string path)
{
    if (!IsExist(path))
        return true;

    auto unc = UncHelper(path);

    auto uncw=U8ToU16(unc.c_str());
    DWORD attr = GetFileAttributesW((LPCWSTR)uncw.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    if (attr & FILE_ATTRIBUTE_READONLY) {
        attr &= ~FILE_ATTRIBUTE_READONLY;
        if (0 == SetFileAttributesW((LPCWSTR)uncw.c_str(), attr)) {
            return false;
        }
    }
    return true;
}
bool DirUtil::IsExist(std::string path)
{
    auto unc = UncHelper(path);
    auto uncw = U8ToU16(unc.c_str());
    DWORD attr = GetFileAttributesW((LPCWSTR)uncw.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

bool DirUtil::IsDirectory(std::string path)
{
    auto unc = UncHelper(path);
    auto uncw = U8ToU16(unc.c_str());
    DWORD attr = GetFileAttributesW((LPCWSTR)uncw.c_str());
    return ((attr != INVALID_FILE_ATTRIBUTES) && (attr&FILE_ATTRIBUTE_DIRECTORY));
}

bool DirUtil::IsRegular(std::string path)
{
    auto unc = UncHelper(path);
    auto uncw = U8ToU16(unc.c_str());
    DWORD attr = GetFileAttributesW((LPCWSTR)uncw.c_str());
    return ((attr != INVALID_FILE_ATTRIBUTES) && !(attr&FILE_ATTRIBUTE_DIRECTORY));
}

uint64_t DirUtil::FileSize(std::string path)
{
    uint64_t fs = 0;
    auto pathw = U8ToU16(path.c_str());
    HANDLE fh = CreateFileW((LPCWSTR)pathw.c_str(),
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

bool DirUtil::CreateDir(std::string path)
{
    auto full = DirUtil::AbsolutePath(path);
    auto unc = UncHelper(full);
    std::stack<std::string> all;
    all.push(unc);
    while (!all.empty()) {
        auto cur = all.top();
        auto curw=U8ToU16(cur.c_str());
        if (CreateDirectoryW((LPCWSTR)curw.c_str(), NULL) == 0) {
            auto err = GetLastError();
            if (err == ERROR_ALREADY_EXISTS) {
                all.pop();
            }
            else if (err == ERROR_PATH_NOT_FOUND) {
                auto pos = cur.find_last_of("\\");
                if (pos != std::string::npos) {
                    all.push(cur.substr(0, pos));
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            all.pop();
        }
    }
    return true;

    //if (SHCreateDirectoryExW(NULL, (LPCWSTR)U8ToU16(dir).c_str(), NULL) == 0/*ERROR_SUCCESS*/) {
    //    return true;
    //}
    //else {
    //    return false;
    //}
}

bool DirUtil::Delete(std::string path)
{
    auto unc = UncHelper(path);
    auto uncw=U8ToU16(unc.c_str());
    return DeleteFileW((LPCWSTR)uncw.c_str()) == TRUE;
}

std::string DirUtil::AbsolutePath(std::string path)
{
    std::vector<wchar_t> buffer(32767);
    auto unc = UncHelper(path);
    auto uncw = U8ToU16(unc.c_str());
    if (0 == GetFullPathNameW((LPCWSTR)uncw.c_str(), (DWORD)buffer.size(), &buffer[0], NULL))
        return std::string();

    return U16ToU8((char16_t*) &buffer[0]);
}

std::string DirUtil::BasePath(std::string path)
{
    auto pos = path.find_last_of('\\');
    return path.substr(0, pos);
}

std::string DirUtil::FileName(std::string path)
{
    auto pos = path.find_last_of('\\');
    return path.substr(pos + 1);
}

bool DirUtil::IterateDir(std::string path)
{

    for (auto const& dir_entry : std::filesystem::directory_iterator{ (char8_t*)path.c_str() })
    {
        if (dir_entry.is_directory()) {
            entries_.emplace_back(DirEntry{ (char*)dir_entry.path().filename().u8string().c_str(), 0, true });
        }
        else {
            entries_.emplace_back(DirEntry{(char*) dir_entry.path().filename().u8string().c_str(), (uint64_t)dir_entry.file_size(), false });
        }
    }


    //auto unc = UncHelper(path) + "\\*";
    //WIN32_FIND_DATAW wfd;
    //auto uncw=U8ToU16(unc.c_str());
    //HANDLE hf = FindFirstFileW((LPCWSTR)uncw.c_str(), &wfd);

    //if (INVALID_HANDLE_VALUE == hf) {
    //    return false;
    //}
    //do {
    //    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    //        if (wcscmp(wfd.cFileName, L".") != 0 &&
    //            wcscmp(wfd.cFileName, L"..") != 0)
    //        entries_.emplace_back(DirEntry{ U16ToU8((char16_t* )wfd.cFileName), 0, true });
    //    }
    //    else {
    //        LARGE_INTEGER size;
    //        size.LowPart = wfd.nFileSizeLow;
    //        size.HighPart = wfd.nFileSizeHigh;
    //        entries_.emplace_back(DirEntry{ U16ToU8((char16_t*)wfd.cFileName), (uint64_t)size.QuadPart, false });
    //    }
    //} while (FindNextFileW(hf, &wfd) != 0);
    //FindClose(hf);
    return true;
}

size_t DirUtil::EntryCount()
{
    return entries_.size();
}

DirEntry DirUtil::GetEntry(size_t index)
{
    assert(index < entries_.size());
    return entries_[index];
}

void DirUtil::ClearDir()
{
    entries_.clear();
}
#else

bool DirUtil::IsExist(std::string path)
{
    struct stat s;
    int err = stat(path.c_str(), &s);
    return err == 0;
}

bool DirUtil::IsDirectory(std::string path)
{
    struct stat s;
    int err = stat(path.c_str(), &s);
    if (err == -1)
        return false;

    return S_ISDIR(s.st_mode);
}

bool DirUtil::IsRegular(std::string path)
{
    struct stat s;
    int err = stat(path.c_str(), &s);
    if (err == -1)
        return false;

    return S_ISREG(s.st_mode);
}

bool DirUtil::CreateDir(std::string path)
{
    const char* dir = path.c_str();

    struct stat s;
    if (stat(dir, &s) == 0 && S_ISDIR(s.st_mode)) {
        return true;
    }
    else {
        return !mkdir(dir, S_IRWXU);
    }
}

std::string DirUtil::AbsolutePath(std::string path)
{
    return "";
}

std::string DirUtil::BasePath(std::string path)
{
    return "";
}

std::string DirUtil::FileName(std::string path)
{
    return "";
}

bool DirUtil::Delete(std::string path)
{
    return false;
}

uint64_t DirUtil::FileSize(std::string path)
{
    struct stat s;
    int err = stat(path.c_str(), &s);
    if (err == -1)
        return 0;

    return s.st_size;
}

bool DirUtil::IterateDir(std::string path)
{
    for (auto const& dir_entry : std::filesystem::directory_iterator{ (char8_t*)path.c_str() })
    {
        if (dir_entry.is_directory()) {
            entries_.emplace_back(DirEntry{ (char*)dir_entry.path().filename().u8string().c_str(), 0, true });
        }
        else {
            entries_.emplace_back(DirEntry{ (char*)dir_entry.path().filename().u8string().c_str(), (uint64_t)dir_entry.file_size(), false });
        }
    }
    //LOG_DEBUG("IterateDir {}  ", path);
    //ClearDir();
    //DIR* dir = opendir(path.c_str());
    //if (dir == NULL) {
    //    LOG_DEBUG("opendir  failed ");
    //    return false;
    //}

    //dirent* ent = NULL;
    //while ((ent = readdir(dir)) != NULL) {
    //    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
    //        continue;
    //    
    //    if (IsDirectory(ent->d_name)) {
    //        LOG_DEBUG("iter dir {} ", ent->d_name);
    //        entries_.emplace_back(DirEntry{ ent->d_name, 0, true });
    //    }
    //    else if (IsRegular(ent->d_name)) {
    //        LOG_DEBUG("iter file {} ", ent->d_name);
    //        auto size = FileSize(ent->d_name);
    //        entries_.emplace_back(DirEntry{ ent->d_name, size, false });
    //    }
    //}

    //closedir(dir);

    return true;
}

size_t DirUtil::EntryCount()
{
    return entries_.size();
}

DirEntry DirUtil::GetEntry(size_t index)
{
    assert(index < entries_.size());
    return entries_[index];
}

void DirUtil::ClearDir()
{
    entries_.clear();
}
std::string DirUtil::UncHelper(std::string path)
{
    return path;
}
#endif

