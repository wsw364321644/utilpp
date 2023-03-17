/**
 * map_file.h
 */

#pragma once

#include <stdint.h>
#include <string>

#include <defs.h>

class CMapFile
{
public:
    CMapFile();
    ~CMapFile();

    int32_t Open(const char *lpFileName);
    void Close();
    int32_t Read(void *pBuf, uint64_t uBegin, uint32_t size);
    int32_t Write(const void *pBuf, uint64_t uBegin, uint32_t size);
    bool IsOpen();

private:
#ifdef WIN32
    F_HANDLE handle_;
#else
    F_HANDLE m_hMappingFile;
#endif // WIN32

    std::string name_;
    uint32_t chunk_size_;
    uint64_t file_size_;
};
