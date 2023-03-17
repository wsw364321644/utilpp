/**
 *  map_file.cpp
 */

#include "map_file.h"
#include "dir_util.h"
#include "logger.h"
#include "string_convert.h"
#ifdef WIN32
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32

const uint32_t FILE_CHUNK_SIZE = 4 * 1024 * 1024;

#ifdef WIN32
CMapFile::CMapFile()
{
    handle_ = INVALID_HANDLE_VALUE;
    chunk_size_ = FILE_CHUNK_SIZE;
    file_size_ = 0;
}

CMapFile::~CMapFile()
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
    }
}

int32_t CMapFile::Open(const char *file_name)
{
    auto unc = DirUtil::UncHelper(file_name);
    auto uncw = U8ToU16(unc.c_str());
    HANDLE map_file = CreateFileW((LPCWSTR)uncw.c_str(),
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                  NULL);

    if (map_file == INVALID_HANDLE_VALUE)
    {
        LOG_WARNING("Can't open the file: {}, error code: {}", file_name, GetLastError());
        return ERR_FILE;
    }

    LARGE_INTEGER size;
    if (GetFileSizeEx(map_file, &size))
    {
        file_size_ = size.QuadPart;
    }
    else
    {
        LOG_WARNING("Failed to get file size: {}, error code: {}", file_name, GetLastError());
        CloseHandle(map_file);
        return ERR_FILE;
    }

    name_ = file_name;

    handle_ = map_file; /*CreateFileMappingW(map_file, NULL, PAGE_READWRITE, size.HighPart, size.LowPart, NULL);

     if (handle_ == INVALID_HANDLE_VALUE) {
         LOG_WARNING("Can't create the file mapping: {}, error code: {}", name_, GetLastError());
         CloseHandle(map_file);
         return ERR_FILE;
     }

     CloseHandle(map_file);
     */
    return ERR_SUCCESS;
}

static void *MapFileBuf(uint64_t begin, uint32_t size, uint32_t chunk, F_HANDLE handle, uint32_t access, int32_t *offset)
{
    int64_t map_begin = (begin / chunk) * chunk;
    uint32_t map_size = static_cast<uint32_t>(begin % chunk) + size;

    uint32_t begin_high = ((uint64_t)map_begin) >> 32;
    uint32_t begin_low = (uint32_t)(((uint64_t)map_begin) & 0xffffffff);
    *offset = (int32_t)(begin - map_begin);
    auto buf = MapViewOfFile(handle, access, begin_high, begin_low, map_size);
    if (buf == NULL)
    {
        LOG_ERROR("MapViewOfFile failed, error code: {}, being:{}, size:{}, map_high: {}, map_low: {}, map_size: {}", GetLastError(), begin, size, begin_high, begin_low, map_size);
    }
    return buf;
}

int32_t CMapFile::Read(void *buf, uint64_t begin, uint32_t size)
{
    assert(begin + size < file_size_);
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_FAILED;
    }

    /*int32_t offset = 0;
    auto map_buf = (char*)MapFileBuf(begin, size, chunk_size_, handle_, FILE_MAP_READ, &offset);
    if (map_buf == NULL) {
        return ERR_FILE;
    }

    memcpy(buf, map_buf + offset, size);
    UnmapViewOfFile(map_buf);*/
    LARGE_INTEGER li;
    li.QuadPart = begin;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(handle_, li.LowPart, &li.HighPart, FILE_BEGIN))
    {
        LOG_WARNING("Failed to seek for reading, file:{}, position:{}, error:{}", name_, begin, GetLastError());
        return ERR_FILE;
        DWORD readed = 0;
        if (!ReadFile(handle_, buf, size, &readed, NULL))
        {
            LOG_WARNING("Faield to read, file:{}, postion:{}, size:{}, error:{}", name_, begin, size, GetLastError());
            return ERR_FILE;
        }
    }

    return ERR_SUCCESS;
}

int32_t CMapFile::Write(const void *buf, uint64_t begin, uint32_t size)
{
    assert(begin + size < file_size_);

    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return ERR_FAILED;
    }
    /*int32_t offset = 0;
    auto map_buf = (char*)MapFileBuf(begin, size, chunk_size_, handle_, FILE_MAP_WRITE, &offset);
    if (map_buf == NULL) {
        return ERR_FILE;
    }

    memcpy(map_buf + offset, buf, size);
    UnmapViewOfFile(buf);
    */
    LARGE_INTEGER li;
    li.QuadPart = begin;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(handle_, li.LowPart, &li.HighPart, FILE_BEGIN))
    {
        LOG_WARNING("Failed to seek for writing, file:{}, position:{}, error:{}", name_, begin, GetLastError());
        return ERR_FILE;
    }
    DWORD written = 0;
    if (!WriteFile(handle_, buf, size, &written, NULL))
    {
        LOG_WARNING("Faield to write, file:{}, postion:{}, size:{}, error:{}", name_, begin, size, GetLastError());
        return ERR_FILE;
    }

    return ERR_SUCCESS;
}

bool CMapFile::IsOpen()
{
    return (handle_ != INVALID_HANDLE_VALUE);
}

void CMapFile::Close()
{
    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
}

#else
CMapFile::CMapFile()
{
    m_hMappingFile = -1;
}

CMapFile::~CMapFile()
{
    Close();
}

int32_t CMapFile::Open(const char *lpFileName)
{
    m_hMappingFile = open(lpFileName, O_RDWR);

    if (m_hMappingFile == -1)
    {
        return ERR_FILE;
    }
    return ERR_SUCCESS;
}

int32_t CMapFile::Read(void *pBuf, uint64_t uBegin, uint32_t size)
{
    if (!IsOpen())
    {
        return ERR_FAILED;
    }

    void *pMem = mmap(NULL, size, PROT_READ, MAP_SHARED, m_hMappingFile, uBegin);
    if (pMem == MAP_FAILED)
        return ERR_FILE;

    memcpy(pBuf, pMem, size);
    munmap(pMem, size);

    return ERR_SUCCESS;
}

int32_t CMapFile::Write(const void *pBuf, uint64_t uBegin, uint32_t size)
{
    if (!IsOpen())
    {
        return ERR_FAILED;
    }

    void *pMem = mmap(NULL, size, PROT_WRITE, MAP_SHARED, m_hMappingFile, uBegin);
    if (pMem == MAP_FAILED)
        return ERR_FILE;

    memcpy(pMem, pBuf, size);
    munmap(pMem, size);
    return ERR_SUCCESS;
}

bool CMapFile::IsOpen()
{
    return (m_hMappingFile != -1);
}

void CMapFile::Close()
{
    if (IsOpen())
    {
        close(m_hMappingFile);
        m_hMappingFile = -1;
    }
}
#endif
