#include "sm_util.h"
#include <uv.h>
#include <LoggerHelper.h>

typedef struct WindowsHandle_t :public CommonHandle_t
{
    WindowsHandle_t(HANDLE _handle): HMapFile(_handle)
    {
        if (HMapFile == NULL) {
            CommonHandle_t(NullHandle);
        }
        else {
            CommonHandle_t(WindowsHandleCount);
        }
    }

    HANDLE HMapFile{NULL};
    size_t FileSize{0};
    std::string Name;
    static std::atomic<CommonHandleID_t> WindowsHandleCount;
} WindowsHandle_t;
std::atomic<WindowsHandle_t::CommonHandleID_t> WindowsHandle_t::WindowsHandleCount{0};

CommonHandle_t* CreateSharedMemory(const char* name, size_t len)
{
    //int flags = UV_FS_O_RDWR | UV_FS_O_CREAT | UV_FS_O_TRUNC | UV_FS_O_FILEMAP;
    //int mode = S_IREAD | S_IWRITE;
    //CommonHandle_t out{ 0 };
    //uv_fs_t req;
    //int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    //uv_fs_req_cleanup(&req);
    //if (res <= 0) {
    //    SIMPLELOG_LOGGER_ERROR(nullptr,"uv_fs_open error: {}\n", uv_strerror(res));
    //    return out;
    //}
    //out = req.result;
    //return  out;
    LARGE_INTEGER li;
    li.QuadPart = len;
    DWORD low = li.LowPart;
    DWORD high = li.HighPart;
    HANDLE HMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // use paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        high,                       // maximum object size (high-order DWORD)
        low,                // maximum object size (low-order DWORD)
        name);                 // name of mapping object

    if (HMapFile == NULL)
    {
        SIMPLELOG_LOGGER_ERROR(nullptr, "Could not create file mapping object({}).\n", GetLastError());
        return nullptr;
    }
    auto handle = new WindowsHandle_t(HMapFile);
    handle->Name = name;
    handle->FileSize = len;
    return handle;
}

CommonHandle_t* OpenSharedMemory(const char* name) {
    //int flags = UV_FS_O_RDWR | UV_FS_O_FILEMAP| UV_FS_O_CREAT;
    //int mode = S_IREAD | S_IWRITE;
    //CommonHandle_t out{ 0 };
    //uv_fs_t req;
    //int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    //uv_fs_req_cleanup(&req);
    //if (res <= 0) {
    //    SIMPLELOG_LOGGER_ERROR(nullptr,"uv_fs_open error: {}\n", uv_strerror(res));
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
        SIMPLELOG_LOGGER_ERROR(nullptr, "Could not open file mapping object({}).", GetLastError());
        return nullptr;
    }
    auto out = new WindowsHandle_t(hMapFile);
    out->Name = name;
    return out;

}

void* MapSharedMemory(CommonHandle_t* phandle)
{
    WindowsHandle_t* handle = dynamic_cast<WindowsHandle_t*>(phandle);
    if (!handle) {
        return  nullptr;
    }
    LPVOID pBuf = MapViewOfFile(handle->HMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        handle->FileSize);
    return pBuf;
}

void* MapReadSharedMemory(CommonHandle_t* phandle)
{
    WindowsHandle_t* handle = dynamic_cast<WindowsHandle_t*>(phandle);
    if (!handle) {
        return  nullptr;
    }
    LPVOID pBuf = MapViewOfFile(handle->HMapFile,   // handle to map object
        FILE_MAP_READ, // read/write permission
        0,
        0,
        handle->FileSize);
    return pBuf;
}

void UnmapSharedMemory(void* ptr)
{
    UnmapViewOfFile(ptr);
}

bool WriteSharedMemory(CommonHandle_t* phandle, void* content, size_t len)
{
    if (!phandle||!phandle->IsValid()) {
        return false;
    }
    //uv_buf_t buf = uv_buf_init((char*)content, *len);
    //uv_fs_t write_req;
    //int res = uv_fs_write(uv_default_loop(), &write_req, handle.ID, &buf, 1, 0, NULL);
    //uv_fs_req_cleanup(&write_req);
    //if (res != *len) {
    //    SIMPLELOG_LOGGER_ERROR(nullptr,"uv_fs_write error: {}\n", uv_strerror(res));
    //}
    //*len = write_req.result;
    

    WindowsHandle_t* handle = dynamic_cast<WindowsHandle_t*>(phandle);
    if (!handle) {
        return  false;
    }
    if (handle->FileSize < len) {
        return false;
    }

    LPTSTR pBuf = (LPTSTR)MapViewOfFile(handle->HMapFile,   // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,
        0,
        len);

    if (pBuf == NULL)
    {
        SIMPLELOG_LOGGER_ERROR(nullptr, "Could not map view of file({}).\n", GetLastError());
        return false;
    }


    CopyMemory((PVOID)pBuf, content, len);
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
    //    SIMPLELOG_LOGGER_ERROR(nullptr,"uv_fs_write error: {}\n", uv_strerror(res));
    //}
    //*len = read_req.result;
    WindowsHandle_t* handle = dynamic_cast<WindowsHandle_t*>(phandle);
    if (!handle) {
        return  false;
    }
    LPTSTR pBuf = (LPTSTR)MapViewOfFile(handle->HMapFile, // handle to map object
        FILE_MAP_ALL_ACCESS,  // read/write permission
        0,
        0,
        *len);

    if (pBuf == NULL)
    {
        SIMPLELOG_LOGGER_ERROR(nullptr, "Could not map view of file({}).", GetLastError());
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
    WindowsHandle_t* handle = dynamic_cast<WindowsHandle_t*>(phandle);
    if (!handle) {
        return;
    }
    if (handle->HMapFile)
        CloseHandle(handle->HMapFile);
    delete handle;
}