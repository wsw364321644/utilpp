/**
 *  raw_file.h
 */

#pragma once

#include <os_defs.h>
#include <memory>
#include <string>

class CRawFile
{
public:
    CRawFile();
    ~CRawFile();

    int32_t Open(const char *lpFileName, uint32_t uOpenFlag ,uint64_t uExpectSize=0);
    int32_t Read(void *pBuf, uint32_t size);
    int32_t Write(const void *pBuf, uint32_t size);
    int32_t Seek(uint64_t uPos);
    uint64_t Tell();
    void Flush();
    void Close();
    int32_t Delete();
    bool IsOpen();
    uint64_t GetSize();
    F_HANDLE GetHandle();
    const char *GetFileName();

private:
    F_HANDLE handle_;
    std::string name_;
    uint64_t file_size_;
};

typedef std::shared_ptr<CRawFile> CRawFilePtr;
