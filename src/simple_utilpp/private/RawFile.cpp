#include "RawFile.h"
#include "logger_header.h"
#include "string_convert.h"
#include "dir_util.h"
#include <filesystem>
#include <format>
#include <system_error>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else

#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32


int32_t FRawFile::InternalOpen(uint32_t uOpenFlag, uint64_t uExpectSize)
{
    auto& filePath = *pFilePath;
    Close();
    DWORD err = 0;

    handle_ = DirUtil::RecursiveCreateFile(filePath, uOpenFlag);
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_FILE;
    }

    if (!DirUtil::SetWritable(filePath))
    {
        err = GetLastError();
        SIMPLELOG_LOGGER_WARN(nullptr, "Failed to set writable, file: {}, error: {}", file_name, err);
        return ERR_FILE;
    }

    if ((uOpenFlag == UTIL_CREATE_ALWAYS || uOpenFlag == UTIL_OPEN_ALWAYS) && (file_size_ != uExpectSize))
    {
        // set the physic size
        if (uExpectSize != 0 && ERR_SUCCESS == Seek(uExpectSize - 1))
        {
            uint8_t uDummy = 0;
            if (ERR_SUCCESS != Write(&uDummy, 1))
            {
                err = GetLastError();
                SIMPLELOG_LOGGER_WARN(nullptr, "Can't create the file: {} with size:{}, ErrorCode is {}", file_name, expect_size, err);
                CloseHandle(handle_);
                handle_ = INVALID_HANDLE_VALUE;
                return ERR_FILE;
            }
            file_size_ = uExpectSize;
            SetEndOfFile(handle_);
        }
        Seek(0);
    }

    return ERR_SUCCESS;
}

int32_t FRawFile::Open(FPathBuf& pathBuf, uint32_t uOpenFlag, uint64_t uExpectSize)
{
    auto& filePath = *pFilePath;
    pathBuf.ToPathW();
    if (pathBuf.PathLenW == 0)
    {
        return ERR_ARGUMENT;
    }
    filePath.SetPathW(pathBuf.GetBufW(), pathBuf.PathLenW);
    return InternalOpen(uOpenFlag, uExpectSize);

}

int32_t FRawFile::Open(std::u8string_view lpFileName, uint32_t uOpenFlag, uint64_t uExpectSize)
{
    auto& filePath = *pFilePath;
    if (lpFileName.size() == 0)
    {
        return ERR_ARGUMENT;
    }
    filePath.SetPath(ConvertU8ViewToView(lpFileName).data(), lpFileName.size());
    filePath.ToPathW();
    return InternalOpen(uOpenFlag, uExpectSize);
}

const char* FRawFile::GetFilePath()
{
    auto& filePath = *pFilePath;
    filePath.ToPath();
    return filePath.GetBuf();
}

#ifdef WIN32

FRawFile::FRawFile()
{
    pFilePath = new FPathBuf;
    handle_ = INVALID_HANDLE_VALUE;
    fd = -1;
    file_size_ = 0;
}

FRawFile::~FRawFile()
{
    Close();
    delete pFilePath;
}

bool FRawFile::IsOpen()
{
    return (handle_ != INVALID_HANDLE_VALUE)|| fd!= -1;
}

int32_t FRawFile::Read(void* buf, uint32_t size)
{
    if (buf == NULL || handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    DWORD readed = 0;
    BOOL res = ReadFile(handle_, buf, size, &readed, NULL);
    if (FALSE == res || readed != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Read file: {} error ...", GetFilePath());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t FRawFile::Write(const void* buf, uint32_t size)
{
    if (buf == NULL || handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    DWORD written = 0;
    BOOL res = WriteFile(handle_, buf, size, &written, NULL);
    if (FALSE == res || written != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Write file: {} error ...", GetFilePath());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t FRawFile::Delete()
{
    auto& filePath = *pFilePath;
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
    if (!DirUtil::Delete(filePath))
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't delete the file : {}", GetFilePath());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

void FRawFile::Close()
{
    if (fd!= -1) {
        close(fd);
        fd = -1;
    }else if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

int32_t FRawFile::Seek(uint64_t pos)
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    int64_t iPos = (int64_t)(pos);

    if (iPos < 0 && pos > file_size_)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Wrong position({}/{}) to seek.", pos, file_size_);
        return ERR_ARGUMENT;
    }

    LONG low = iPos & 0xffffffff;
    LONG high = iPos >> 32;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(handle_, low, &high, FILE_BEGIN))
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Seek the file {} error ... error code: {}", GetFilePath(), GetLastError());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

uint64_t FRawFile::Tell()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_ARGUMENT;
    }

    LONG high = 0;
    uint32_t low = SetFilePointer(handle_, 0, &high, FILE_CURRENT);
    if (INVALID_SET_FILE_POINTER == low)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Tell the file {} error ... error code: {}", GetFilePath(), GetLastError());
        return ERR_FILE;
    }

    return (static_cast<uint64_t>(low) | (static_cast<uint64_t>(high) << 32));
}

void FRawFile::Flush()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return;
    }

    BOOL res = FlushFileBuffers(handle_);
    if (res == FALSE)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't flush file buffer, error code: {}", GetLastError());
    }
}

uint64_t FRawFile::GetSize()
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    DWORD err = 0;
    DWORD low = 0;
    DWORD high = 0;
    low = GetFileSize(handle_, &high);

    if (low == 0xffffffff && (err = GetLastError()) != NO_ERROR)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, "Can't get the size of file: {}. ErrorCode is {}", GetFilePath(), err);
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return 0;
    }

    file_size_ = ((uint64_t)high << 32) | ((uint64_t)low);
    return file_size_;
}
F_HANDLE FRawFile::GetHandle()
{
    if (fd!= -1) {
        return (F_HANDLE)_get_osfhandle(fd);
    }
    return handle_;
}

int FRawFile::GetFD()
{
    if (handle_!= INVALID_HANDLE_VALUE) {
        fd= _open_osfhandle(reinterpret_cast<intptr_t>(handle_), _O_RDWR);
        handle_ = INVALID_HANDLE_VALUE;
    }
    return fd;
}

#else
FRawFile::FRawFile()
{
    handle_ = INVALID_HANDLE_VALUE;
    fd = -1;
    file_size_ = 0;
}

FRawFile::~FRawFile()
{
    Close();
}

int32_t MakeDir(const char* lpPath)
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

int32_t FRawFile::Open(const char8_t* lpFileName, uint32_t uOpenFlag, uint64_t uExpectSize)
{
    if (lpFileName == NULL)
    {
        return ERR_ARGUMENT;
    }
    name_ = (const char*)lpFileName;

    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    handle_ = open(name_.c_str(), uOpenFlag, filePerms);
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        if (errno != ENOENT)
            return ERR_FILE;
        std::filesystem::path curPath(name_);
        if (ERR_SUCCESS != MakeDir((const char*)curPath.parent_path().u8string().c_str()))
            return ERR_FILE;
        handle_ = open(name_.c_str(), uOpenFlag, filePerms);
        if (handle_ == INVALID_HANDLE_VALUE)
            return ERR_FILE;
    }
    file_size_ = std::filesystem::file_size(name_);
    if ((uOpenFlag == UTIL_CREATE_ALWAYS || uOpenFlag == UTIL_OPEN_ALWAYS) && (file_size_ != uExpectSize))
    {
        // make file as desized size
        if (-1 == truncate(name_.c_str(), uExpectSize))
        {
            SIMPLELOG_LOGGER_ERROR(nullptr, std::format("failed to set file size, file:{}, size:{}, error:{}", name_.c_str(), uExpectSize, errno));
            return ERR_FILE;
        }
        file_size_ = uExpectSize;
    }

    return ERR_SUCCESS;
}

bool FRawFile::IsOpen()
{
    return (handle_ != INVALID_HANDLE_VALUE);
}

int32_t FRawFile::Read(void* pBuf, uint32_t size)
{
    if (pBuf == NULL || !IsOpen())
    {
        return ERR_ARGUMENT;
    }

    auto readed = read(handle_, pBuf, size);
    if (readed < 0 or readed != size)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, std::format("Read file: {} error ...", GetFilePath()));
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

int32_t FRawFile::Write(const void* pBuf, uint32_t size)
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

int32_t FRawFile::Delete()
{
    if (!IsOpen())
    {
        return ERR_ARGUMENT;
    }

    close(handle_);
    handle_ = -1;
    if (0 == unlink(GetFilePath()))
    {
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

void FRawFile::Close()
{
    if (!IsOpen())
    {
        return;
    }

    close(handle_);
    handle_ = -1;
}

int32_t FRawFile::Seek(uint64_t uPos)
{
    if (!IsOpen())
    {
        return ERR_ARGUMENT;
    }

    if (uPos > file_size_)
    {
        SIMPLELOG_LOGGER_WARN(nullptr, std::format("Wrong position({}/{}) to seek.", uPos, file_size_));
        return ERR_ARGUMENT;
    }

    if (lseek(handle_, uPos, SEEK_SET) < 0)
    {
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

uint64_t FRawFile::Tell()
{
    if (handle_ == INVALID_HANDLE_VALUE)
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

void FRawFile::Flush()
{
}

uint64_t FRawFile::GetSize()
{
    return file_size_;
}
F_HANDLE FRawFile::GetHandle()
{
    return handle_;
}

int FRawFile::GetFD()

    return handle_;
}
#endif

