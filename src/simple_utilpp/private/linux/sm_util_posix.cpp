#include "sm_util.h"
#include <uv.h>
#include <LoggerHelper.h>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
typedef struct PosixHandle_t :public CommonHandle_t
{
    PosixHandle_t(int  _handle): HMapFile(_handle)
    {
        if (HMapFile == -1) {
            CommonHandle_t(NullHandle);
        }
        else {
            CommonHandle_t(PosixHandleCount);
        }
    }

    int  HMapFile{0};
    size_t FileSize{0};
    std::string Name;
    static std::atomic<CommonHandleID_t> PosixHandleCount;
} PosixHandle_t;
std::atomic<PosixHandle_t::CommonHandleID_t> PosixHandle_t::PosixHandleCount{0};

CommonHandle_t* CreateSharedMemory(const char* name, size_t len, std::error_code& ec)
{
    key_t key = ftok(name, 65);
    if(key==-1){
        ec = std::error_code(errno, std::system_category());
        return NULL;
    }
    int shmid = shmget(key, len, 0666|IPC_CREAT|IPC_EXCL);
    if(shmid==-1){
        ec = std::error_code(errno, std::system_category());
        return NULL;
    }
    auto handle = new PosixHandle_t(shmid);
    handle->Name = name;
    handle->FileSize = len;
    ec.clear();
    return handle;
}

CommonHandle_t* OpenSharedMemory(const char* name, std::error_code& ec) {
    key_t key = ftok("shmfile", 65);
    if(key==-1){
        ec = std::error_code(errno, std::system_category());
        return NULL;
    }
    int shmid = shmget(key, 0, 0666);
    if(shmid==-1){
        ec = std::error_code(errno, std::system_category());
        return NULL;
    }
    auto out = new PosixHandle_t(shmid);
    out->Name = name;
    ec.clear();
    return out;

}

void* MapSharedMemory(CommonHandle_t* phandle)
{
    PosixHandle_t* handle = dynamic_cast<PosixHandle_t*>(phandle);
    if (!handle) {
        return  nullptr;
    }
    void *pBuf =  shmat(handle->HMapFile, (void*)0, 0);
    if(pBuf==(void *) -1){
        return NULL;
    }
    return pBuf;
}

void* MapReadSharedMemory(CommonHandle_t* phandle)
{
    PosixHandle_t* handle = dynamic_cast<PosixHandle_t*>(phandle);
    if (!handle) {
        return  nullptr;
    }
    void *pBuf =  shmat(handle->HMapFile, (void*)0, SHM_RDONLY);
        if(pBuf==(void *) -1){
        return NULL;
    }
    return pBuf;
}

void UnmapSharedMemory(void* ptr)
{
    shmdt(ptr);
}

bool WriteSharedMemory(CommonHandle_t* phandle, void* content, size_t len, std::error_code& ec)
{
    if (!phandle||!phandle->IsValid()) {
        ec=std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    PosixHandle_t* handle = dynamic_cast<PosixHandle_t*>(phandle);
    if (!handle) {
        ec=std::make_error_code(std::errc::invalid_argument);
        return  false;
    }
    if (handle->FileSize < len) {
        ec=std::make_error_code(std::errc::invalid_argument);
        return false;
    }
    void* pBuf = MapSharedMemory(phandle);
    if (pBuf == NULL)
    {
        ec = std::error_code(errno, std::system_category());
        return false;
    }
    memcpy(pBuf, content, len);
    UnmapSharedMemory(pBuf);
    ec.clear();
    return true;
}

bool ReadSharedMemory(CommonHandle_t* phandle, void* content, size_t* len, std::error_code& ec)
{
    if (!phandle || !phandle->IsValid()) {
        ec=std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    PosixHandle_t* handle = dynamic_cast<PosixHandle_t*>(phandle);
    if (!handle) {
        ec=std::make_error_code(std::errc::invalid_argument);
        return  false;
    }
    void* pBuf = MapReadSharedMemory(phandle);

    if (pBuf == NULL)
    {
        ec = std::error_code(errno, std::system_category());
        return false;
    }

    memcpy( content,pBuf, *len);
    UnmapSharedMemory(pBuf);
    ec.clear();
    return true;
}

void CloseSharedMemory(CommonHandle_t* phandle)
{
    if (!phandle || !phandle->IsValid()) {
        return;
    }
    PosixHandle_t* handle = dynamic_cast<PosixHandle_t*>(phandle);
    if (!handle) {
        return;
    }
    if (handle->HMapFile)
        shmctl(handle->HMapFile, IPC_RMID, NULL);
    delete handle;
}