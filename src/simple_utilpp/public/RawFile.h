#pragma once

#include <simple_os_defs.h>
#include <memory>
#include <string>
#include "PathBuf.h"
#include "simple_export_ppdefs.h"

class SIMPLE_UTIL_EXPORT FRawFile :public TProvideThreadSingletonClass<FRawFile>
{
public:
    FRawFile();
    ~FRawFile();

    int32_t Open(FPathBuf& pathBuf, uint32_t uOpenFlag ,uint64_t uExpectSize=0);
    int32_t Open(std::u8string_view lpFileName, uint32_t uOpenFlag ,uint64_t uExpectSize=0);
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
    int GetFD();
    const char *GetFilePath();

private:
    int32_t InternalOpen(uint32_t uOpenFlag, uint64_t uExpectSize);
    F_HANDLE handle_;
    int fd;
    FPathBuf filePath;
    uint64_t file_size_;
};

typedef std::shared_ptr<FRawFile> RawFilePtr;
