#include "sm_util.h"
#include <uv.h>
#include <logger.h>

CommonHandle_t CreateSharedMemory(const char* name) {
    //uv_loop_t loop;
    //uv_loop_init(&loop);
    //uv_fs_t* preq = new uv_fs_t;
    int flags = UV_FS_O_RDWR | UV_FS_O_CREAT | UV_FS_O_TRUNC | UV_FS_O_FILEMAP;
    int mode = S_IREAD | S_IWRITE;
    CommonHandle_t out{ 0 };
    //std::function<void(int)>* pOnSharedMemoryCreated = new std::function<void(int)>([&](int fd) {
    //    {
    //        out =(uv_file) preq->result;
    //    }
    //    });
    //preq->data = pOnSharedMemoryCreated;
    //
    //int res = uv_fs_open(&loop, preq, name, flags, mode,
    //    [](uv_fs_t* preq) {
    //        {
    //            if (preq->result < 0) {
    //                LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(preq->result));
    //            }
    //            else {

    //                std::function<void(int)>* fn = (std::function<void(int)>*)preq->data;
    //                (*fn)(preq->result);
    //            }
    //            delete (std::function<void(int)>*)preq->data;
    //            uv_fs_req_cleanup(preq);
    //            delete preq;
    //        }
    //    });

    //if (res) {
    //    LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(res));
    //}

    //uv_run(&loop, uv_run_mode::UV_RUN_DEFAULT);
    uv_fs_t req;
    int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (res <= 0) {
        LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(res));
        return out;
    }
    out = req.result;
    return  out;
}

CommonHandle_t OpenSharedMemory(const char* name) {
    int flags = UV_FS_O_RDWR | UV_FS_O_FILEMAP;
    int mode = S_IREAD | S_IWRITE;
    CommonHandle_t out{ 0 };
    uv_fs_t req;
    int res = uv_fs_open(uv_default_loop(), &req, name, flags, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (res <= 0) {
        LOG_ERROR("uv_fs_open error: {}\n", uv_strerror(res));
        return out;
    }
    out = req.result;
    return  out;
}

bool WriteSharedMemory(CommonHandle_t handle, void* content, size_t* len)
{
    if (!handle.IsValid()) {
        return false;
    }
    uv_buf_t buf = uv_buf_init((char*)content, *len);

    //uv_fs_t* write_req = new uv_fs_t;
    //auto* pOnSharedMemoryWrote = new std::function<void( size_t)>([&](size_t _len) {
    //    {
    //        *len = _len;
    //    }
    //    });
    //write_req->data = pOnSharedMemoryWrote;
    //// Write to the file descriptor
    //int res =  uv_fs_write(&loop, write_req, handle.ID,
    //    &buf, 1, 0,
    //    [](uv_fs_t* preq) {
    //        {
    //            if (preq->result < 0) {
    //                LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(preq->result));
    //            }

    //            auto pOnSharedMemoryWrote =(std::function<void(size_t)>*) preq->data;
    //            (*pOnSharedMemoryWrote)(preq->result);

    //            free(preq->data);
    //            uv_fs_req_cleanup(preq);
    //            delete preq;

    //        }
    //    });
    uv_fs_t write_req;
    int res = uv_fs_write(uv_default_loop(), &write_req, handle.ID, &buf, 1, 0, NULL);
    uv_fs_req_cleanup(&write_req);
    if (res != *len) {
        LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(res));
    }
    *len = write_req.result;
    //uv_run(&loop, uv_run_mode::UV_RUN_DEFAULT);
    return true;
}
bool ReadSharedMemory(CommonHandle_t handle, void* content, size_t* len)
{
    if (!handle.IsValid()) {
        return false;
    }
    uv_buf_t buf = uv_buf_init((char*)content, *len);
    *len = 0;
    //uv_fs_t* read_req = new uv_fs_t;
    //read_req->data = len;
    //int res = uv_fs_read(&loop, read_req, handle.ID,
    //    &buf, 1, 0,
    //    [](uv_fs_t* preq) {
    //        {
    //            if (preq->result < 0) {
    //                LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(preq->result));
    //            }
    //            else if (preq->result == 0) {
    //                free(preq->data);
    //                uv_fs_req_cleanup(preq);
    //                delete preq;
    //            }
    //            
    //            (std::size_t*)read_req->data
    //        }
    //    });
    uv_fs_t read_req;
    int res = uv_fs_read(uv_default_loop(), &read_req, handle.ID, &buf, 1, 0, NULL);
    uv_fs_req_cleanup(&read_req);
    if (res) {
        LOG_ERROR("uv_fs_write error: {}\n", uv_strerror(res));
    }
    *len = read_req.result;
    //uv_run(&loop, uv_run_mode::UV_RUN_DEFAULT);
    return true;
}
void CloseSharedMemory(CommonHandle_t handle)
{
    if (!handle.IsValid()) {
        return;
    }
    uv_fs_t closeReq;
    uv_fs_close(uv_default_loop(), &closeReq,
        handle.ID,
        NULL);
}