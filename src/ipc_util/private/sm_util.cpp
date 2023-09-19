#include "sm_util.h"
#include <uv.h>
#include <logger.h>

typedef struct WindowsHandle:public CommonHandle
{
    WindowsHandle(HANDLE _handle):CommonHandle(WindowsHandleCount), HMapFile(_handle)
    {
        if (HMapFile == NULL) {
            CommonHandle();
        }
        else {
            CommonHandle(WindowsHandleCount);
        }
    }

    HANDLE HMapFile{NULL};
    size_t FileSize{0};
    std::string Name;
    static std::atomic_uint32_t WindowsHandleCount;
} WindowsHandle_t;
std::atomic_uint32_t WindowsHandle_t::WindowsHandleCount{ 0 };

CommonHandle_t* CreateSharedMemory(const char* name)
{
    //int flags = UV_FS_O_RDWR | UV_FS_O_CREAT | UV_FS_O_TRUNC | UV_FS_O_FILEMAP;
    //int mode = S_IREAD | S_IWRITE;
    //CommonHandle_t out{ 0 };
    //uv_fs_t req;
    //int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    //uv_fs_req_cleanup(&req);
    //if (res <= 0) {
    //    LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(res));
    //    return out;
    //}
    //out = req.result;
    //return  out;

    auto out=new WindowsHandle_t(nullptr) ;
    out->Name = name;
    out->FileSize = 0;
    return out;
}

CommonHandle_t* OpenSharedMemory(const char* name) {
    //int flags = UV_FS_O_RDWR | UV_FS_O_FILEMAP| UV_FS_O_CREAT;
    //int mode = S_IREAD | S_IWRITE;
    //CommonHandle_t out{ 0 };
    //uv_fs_t req;
    //int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    //uv_fs_req_cleanup(&req);
    //if (res <= 0) {
    //    LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(res));
    //    return out;
    //}
    //out = req.result;
    //return  out;
    HANDLE hMapFile;
    
    hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        name);               // name of mapping object

    if (hMapFile == NULL)
    {
        LOG_ERROR("Could not open file mapping object({}).\n", GetLastError());
        return nullptr;
    }
    auto out = new WindowsHandle_t(hMapFile);
    out->Name = name;
    return out;

}

bool WriteSharedMemory(CommonHandle_t* phandle, void* content, size_t* len)
{
    if (!phandle||!phandle->IsValid()) {
        return false;
    }
    //uv_buf_t buf = uv_buf_init((char*)content, *len);
    //uv_fs_t write_req;
    //int res = uv_fs_write(uv_default_loop(), &write_req, handle.ID, &buf, 1, 0, NULL);
    //uv_fs_req_cleanup(&write_req);
    //if (res != *len) {
    //    LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(res));
    //}
    //*len = write_req.result;
    

    WindowsHandle_t* handle = static_cast<WindowsHandle_t*>(phandle);
    if (handle->FileSize < *len) {
        if (handle->HMapFile) {
            CloseHandle(handle->HMapFile);
        }
        handle->HMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            *len,                // maximum object size (low-order DWORD)
            handle->Name.c_str());                 // name of mapping object

        if (handle->HMapFile == NULL)
        {
            LOG_ERROR("Could not create file mapping object({}).\n", GetLastError());
            handle->FileSize = 0;
            return false;
        }
        handle->FileSize = *len;
    }
        

    LPTSTR pBuf = (LPTSTR)MapViewOfFile(handle->HMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        *len);

    if (pBuf == NULL)
    {
        LOG_ERROR("Could not map view of file({}).\n", GetLastError());
        return false;
    }


    CopyMemory((PVOID)pBuf, content, *len);

    UnmapViewOfFile(pBuf);

    return true;
}
bool ReadSharedMemory(CommonHandle_t* phandle, void* content, size_t* len)
{
    if (!phandle || !phandle->IsValid()) {
        return false;
    }
    //uv_buf_t buf = uv_buf_init((char*)content, *len);
    //*len = 0;
    //uv_fs_t read_req;
    //int res = uv_fs_read(uv_default_loop(), &read_req, handle.ID, &buf, 1, 0, NULL);
    //uv_fs_req_cleanup(&read_req);
    //if (res) {
    //    LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(res));
    //}
    //*len = read_req.result;
    WindowsHandle_t* handle = static_cast<WindowsHandle_t*>(phandle);
    LPTSTR pBuf = (LPTSTR)MapViewOfFile(handle->HMapFile, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        *len);

    if (pBuf == NULL)
    {
        LOG_ERROR("Could not map view of file({}).\n", GetLastError());
        return false;
    }

    CopyMemory( content, (PVOID)pBuf, *len);

    UnmapViewOfFile(pBuf);



    return true;
}
void CloseSharedMemory(CommonHandle_t* phandle)
{
    if (!phandle || !phandle->IsValid()) {
        return;
    }
    //uv_fs_t closeReq;
    //uv_fs_close(uv_default_loop(), &closeReq,
    //    handle.ID,
    //    NULL);
    WindowsHandle_t* handle = static_cast<WindowsHandle_t*>(phandle);
    if (handle->HMapFile)
        CloseHandle(handle->HMapFile);
}