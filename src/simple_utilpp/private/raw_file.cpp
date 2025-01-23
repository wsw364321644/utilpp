/**
 *  raw_file.cpp
 */

#include "raw_file.h"
#include "logger_header.h"
#include "string_convert.h"
#include "dir_util.h"
#include <filesystem>
#include <format>
#include <system_error>
#ifdef WIN32
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32

#ifdef WIN32

CRawFile::CRawFile()
{
    handle_ = INVALID_HANDLE_VALUE;
    file_size_ = 0;
}

CRawFile::~CRawFile()
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(INVALID_HANDLE_VALUE);
    }
}

int32_t CRawFile::Open(const char *file_name, uint32_t flag, uint64_t expect_size)
{
    if (file_name == NULL)
    {
        return ERR_ARGUMENT;
    }

    auto unc = DirUtil::UncHelper(file_name);

    DWORD err = 0;
    if (!DirUtil::SetWritable(file_name))
    {
        err = GetLastError();
        SIMPLELOG_LOGGER_WARN(nullptr,"Failed to set writable, file: {}, error: {}", file_name, err);
        return ERR_FILE;
    }
    auto uncw = U8ToU16(unc.c_str());
    handle_ = CreateFileW((LPCWSTR)uncw.c_str(),
                          GENERIC_WRITE | GENERIC_READ,
                          FILE_SHARE_WRITE | FILE_SHARE_READ,
                          NULL,
                          flag,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (handle_ == INVALID_HANDLE_VALUE)
    {
        err = GetLastError();
        // Path not found? Create the directory
        if ((flag == UTIL_CREATE_ALWAYS || flag == UTIL_OPEN_ALWAYS) && err == ERROR_PATH_NOT_FOUND)
        {
            std::filesystem::path base_path = std::filesystem::path((char8_t *)file_name).parent_path();
            std::filesystem::path base_name = std::filesystem::path((char8_t *)file_name).filename();
            if (std::filesystem::exists(base_path) || std::filesystem::create_directories(base_path))
            {
                handle_ = CreateFileW((LPCWSTR)uncw.c_str(),
                                      GENERIC_WRITE | GENERIC_READ,
                                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                                      NULL,
                                      flag,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);
                if (handle_ == INVALID_HANDLE_VALUE)
                {
                    err = GetLastError();
                    SIMPLELOG_LOGGER_WARN(nullptr,"Can't open the file after create directory: {}. ErrorCode is {}", file_name, err);
                    return ERR_FILE;
                }
            }
            else
            {
                err = GetLastError();
                SIMPLELOG_LOGGER_WARN(nullptr,"Failed to create the directory: {}. ErrorCode is {}", (const char *)base_path.u8string().c_str(), err);
                return ERR_FILE;
            }
        }
        else
        {
            err = GetLastError();
            SIMPLELOG_LOGGER_WARN(nullptr,"Can't open the file: {}. ErrorCode is {}", file_name, err);
            return ERR_FILE;
        }
    }

    DWORD low = 0;
    DWORD high = 0;
    low = GetFileSize(handle_, &high);

    if (low == 0xffffffff && (err = GetLastError()) != NO_ERROR)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Can't get the size of file: {}. ErrorCode is {}", file_name, err);
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return ERR_FILE;
    }

    file_size_ = ((uint64_t)high << 32) | ((uint64_t)low);

    if ((flag == UTIL_CREATE_ALWAYS || flag == UTIL_OPEN_ALWAYS) && (file_size_ != expect_size))
    {
        // set the physic size
        if (expect_size != 0 && ERR_SUCCESS == Seek(expect_size - 1))
        {
            uint8_t uDummy = 0;
            if (ERR_SUCCESS != Write(&uDummy, 1))
            {
                err = GetLastError();
                SIMPLELOG_LOGGER_WARN(nullptr,"Can't create the file: {} with size:{}, ErrorCode is {}", file_name, expect_size, err);
                CloseHandle(handle_);
                handle_ = INVALID_HANDLE_VALUE;
                return ERR_FILE;
            }
            file_size_ = expect_size;
            SetEndOfFile(handle_);
        }
    }

    name_ = file_name;
    return ERR_SUCCESS;
}

bool CRawFile::IsOpen()
{
    return (handle_ != INVALID_HANDLE_VALUE);
}

int32_t CRawFile::Read(void *buf, uint32_t size)
{
    if (buf == NULL || handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    DWORD readed = 0;
    BOOL res = ReadFile(handle_, buf, size, &readed, NULL);
    if (FALSE == res || readed != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Read file: {} error ...", name_);
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t CRawFile::Write(const void *buf, uint32_t size)
{
    if (buf == NULL || handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    DWORD written = 0;
    BOOL res = WriteFile(handle_, buf, size, &written, NULL);
    if (FALSE == res || written != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Write file: {} error ...", name_);
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t CRawFile::Delete()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
    if (!DirUtil::Delete(name_))
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Can't delete the file : {}", name_);
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

void CRawFile::Close()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return;
    }

    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
}

int32_t CRawFile::Seek(uint64_t pos)
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    int64_t iPos = (int64_t)(pos);

    if (iPos < 0 && pos > file_size_)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Wrong position({}/{}) to seek.", pos, file_size_);
        return ERR_ARGUMENT;
    }

    LONG low = iPos & 0xffffffff;
    LONG high = iPos >> 32;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(handle_, low, &high, FILE_BEGIN))
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Seek the file {} error ... error code: {}", name_, GetLastError());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

uint64_t CRawFile::Tell()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    LONG high = 0;
    uint32_t low = SetFilePointer(handle_, 0, &high, FILE_CURRENT);
    if (INVALID_SET_FILE_POINTER == low)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Tell the file {} error ... error code: {}", name_, GetLastError());
        return ERR_FILE;
    }

    return (static_cast<uint64_t>(low) | (static_cast<uint64_t>(high) << 32));
}

void CRawFile::Flush()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return;
    }

    BOOL res = FlushFileBuffers(handle_);
    if (res == FALSE)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,"Can't flush file buffer, error code: {}", GetLastError());
    }
}

uint64_t CRawFile::GetSize()
{
    return file_size_;
}

F_HANDLE CRawFile::GetHandle()
{
    return handle_;
}

const char *CRawFile::GetFileName()
{
    return name_.c_str();
}
#else
CRawFile::CRawFile()
{
    handle_ = -1;
    file_size_ = 0;
}

CRawFile::~CRawFile()
{
    if (handle_ != -1)
    {
        close(handle_);
    }
}

int32_t MakeDir(const char *lpPath)
{
    if (lpPath == NULL)
    {
        return ERR_ARGUMENT;
    }
    std::filesystem::path dst_path(lpPath);

    if (std::filesystem::exists(dst_path))
    {
        if (std::filesystem::is_directory(dst_path))
            return ERR_SUCCESS;
        else
            return ERR_FILE;
    }

    std::error_code ec;
    if (std::filesystem::create_directories(dst_path, ec) == false)
    {
        return ERR_FILE;
    }
    else
    {
        return ERR_SUCCESS;
    }
}

int32_t CRawFile::Open(const char *lpFileName, uint32_t uOpenFlag, uint64_t uExpectSize)
{
    if (lpFileName == NULL)
    {
        return ERR_ARGUMENT;
    }
    name_ = lpFileName;

    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    handle_ = open(lpFileName, uOpenFlag, filePerms);
    if (handle_ == -1)
    {
        if (errno != ENOENT)
            return ERR_FILE;
        std::filesystem::path curPath(name_);
        if (ERR_SUCCESS != MakeDir((const char *)curPath.parent_path().u8string().c_str()))
            return ERR_FILE;
        handle_ = open(lpFileName, uOpenFlag, filePerms);
        if (handle_ == -1)
            return ERR_FILE;
    }
    file_size_ = std::filesystem::file_size(name_);
    if ((uOpenFlag == UTIL_CREATE_ALWAYS || uOpenFlag == UTIL_OPEN_ALWAYS) && (file_size_ != uExpectSize))
    {
        // make file as desized size
        if (-1 == truncate(lpFileName, uExpectSize))
        {
            SIMPLELOG_LOGGER_ERROR(nullptr,std::format("failed to set file size, file:{}, size:{}, error:{}", lpFileName, uExpectSize, errno));
            return ERR_FILE;
        }
        file_size_ = uExpectSize;
    }

    return ERR_SUCCESS;
}

bool CRawFile::IsOpen()
{
    return (handle_ != -1);
}

int32_t CRawFile::Read(void *pBuf, uint32_t size)
{
    if (pBuf == NULL || !IsOpen())
    {
        return ERR_ARGUMENT;
    }

    auto readed = read(handle_, pBuf, size);
    if (readed < 0 or readed != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,std::format("Read file: {} error ...", name_));
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t CRawFile::Write(const void *pBuf, uint32_t size)
{
    if (pBuf == NULL || !IsOpen())
    {
        return ERR_ARGUMENT;
    }

    auto written = write(handle_, pBuf, size);
    if (written < 0 || written != size)
    {
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t CRawFile::Delete()
{
    if (!IsOpen())
    {
        return ERR_ARGUMENT;
    }

    close(handle_);
    handle_ = -1;
    if (0 == unlink(name_.c_str()))
    {
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

void CRawFile::Close()
{
    if (!IsOpen())
    {
        return;
    }

    close(handle_);
    handle_ = -1;
}

int32_t CRawFile::Seek(uint64_t uPos)
{
    if (!IsOpen())
    {
        return ERR_ARGUMENT;
    }

    if (uPos > file_size_)
    {
        SIMPLELOG_LOGGER_WARN(nullptr,std::format("Wrong position({}/{}) to seek.", uPos, file_size_));
        return ERR_ARGUMENT;
    }

    if (lseek(handle_, uPos, SEEK_SET) < 0)
    {
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

uint64_t CRawFile::Tell()
{
    if (handle_ == -1)
    {
        return ERR_ARGUMENT;
    }

    auto uPos = lseek(handle_, 0, SEEK_CUR);
    if (uPos < 0)
    {
        return ERR_FILE;
    }
    return uPos;
}

void CRawFile::Flush()
{
}

uint64_t CRawFile::GetSize()
{
    return file_size_;
}

int CRawFile::GetHandle()
{
    return handle_;
}

const char *CRawFile::GetFileName()
{
    return name_.c_str();
}

#endif
