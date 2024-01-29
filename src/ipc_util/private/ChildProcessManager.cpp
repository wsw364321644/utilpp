#include "ChildProcessManager.h"
#include <uv.h>
#include <mutex>
#include <logger.h>
#include <string_buffer.h>

typedef struct UVProcess_t {
    ~UVProcess_t() {
        if (args) {
            for (int i = 0; args[i] != 0; i++) {
                free(args[i]);
            }
            free(args);
            args = nullptr;
        }
    }
    bool AllocArgs(size_t s) {
        args = (char**)calloc(s + 1, sizeof(char*));
        return args != nullptr;
    }
    bool AllocArgsIndex(size_t index, size_t s) {
        args[index] = (char*)calloc(s, sizeof(char));
        return args[index] != nullptr;
    }
    char** args{ nullptr };
    uv_process_t process{ 0 };
    uv_process_options_t options{ 0 };
    uv_stdio_container_t child_stdio[4];
    uv_pipe_t pipe;
    uv_async_t async;
    FChildProcessManager* ChildProcessManager;
    FChildProcessManager::FOnReadDelegate  OnReadDelegate;
    FChildProcessManager::FOnExitDelegate  OnExitDelegate;
    CommonHandle_t handle;
    CharBuffer Buf;
    //std::mutex m;
    //std::condition_variable cv;
    //bool running = true;
}UVProcess_t;
//void on_new_connection(uv_stream_t* server, int status) {
//    UVProcess_t* UVProcess = (UVProcess_t*)server->data;
//    UVProcess->ChildProcessManager->OnNewConnection(server, status);
//}

void alloc_buffer(uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf) {
    UVProcess_t& p = *(UVProcess_t*)handle->data;
    p.Buf.Reverse(suggested_size);
    *buf = uv_buf_init((char*)p.Buf.Data(), p.Buf.Size());
};
void on_read(uv_stream_t* stream,
    ssize_t nread,
    const uv_buf_t* buf) {
    UVProcess_t& p = *(UVProcess_t*)stream->data;

    //if (nread < 0) {
    //    p.OnReadDelegate(p.handle, buf->base, nread);
    //}
    //else {
    //    p.OnReadDelegate(p.handle, buf->base, nread);
    //}
    if (p.OnReadDelegate) {
        p.OnReadDelegate(p.handle, buf->base, nread);
    }
};

FChildProcessManager::FChildProcessManager()
{
    ploop = new uv_loop_t;
    uv_loop_init(ploop);
}
FChildProcessManager* FChildProcessManager::GetInstance()
{
    static  FChildProcessManager* ptr{ nullptr };
    if (ptr == nullptr) {
        ptr = new FChildProcessManager;
    }
    return ptr;
}
CommonHandle_t FChildProcessManager::SpawnProcess(const char* _filepath, const char** _args)
{
    if (!_filepath) {
        return CommonHandle_t();
    }
    auto pair = processes.emplace(processCount, std::make_shared< UVProcess_t>());
    if (!pair.second) {
        return CommonHandle_t();
    }
    UVProcess_t* pp = pair.first->second.get();
    pp->ChildProcessManager = this;
    pp->handle = pair.first->first;
    int argc = 1;
    if (_args != nullptr)
    {
        for (int i = 0;; i++) {
            if (_args[i] == 0) {
                break;
            }
            argc++;
        }
    }
    pp->AllocArgs(argc);
    pp->AllocArgsIndex(0, strlen(_filepath) + 1);
    strcpy(pp->args[0], _filepath);
    for (int i = 0; argc - i > 1; i++) {
        pp->AllocArgsIndex(i + 1, strlen(_args[i]) + 1);
        strcpy(pp->args[i + 1], _args[i]);
    }

    return pair.first->first;
}
void FChildProcessManager::RegisterOnRead(CommonHandle_t handle, FOnReadDelegate delegate)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    pair->second->OnReadDelegate = delegate;
}
void FChildProcessManager::RegisterOnExit(CommonHandle_t handle, FOnExitDelegate delegate)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    pair->second->OnExitDelegate = delegate;
}
bool FChildProcessManager::CheckIsFinished(CommonHandle_t handle)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return true;
    }
    return false;
}
void FChildProcessManager::Tick(float delSec)
{
    if (!currentHandle.IsValid()) {
        auto itr = processes.begin();
        if (itr == processes.end()) {
            return;
        }
        currentHandle = itr->first;
        InternalSpawnProcess(itr->second.get());
    }
    uv_run(ploop, uv_run_mode::UV_RUN_ONCE);
}

void FChildProcessManager::Run()
{
    do {
        uv_run(ploop, uv_run_mode::UV_RUN_DEFAULT);
        auto itr = processes.begin();
        if (itr == processes.end()) {
            break;
        }
        currentHandle = itr->first;
        InternalSpawnProcess(itr->second.get());
    } while (currentHandle.IsValid());
}

void FChildProcessManager::ClearProcessData(CommonHandle_t handle)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    auto pp = pair->second.get();
    uv_close((uv_handle_t*)&pp->process, nullptr);
    uv_close((uv_handle_t*)&pp->pipe, nullptr);
    //uv_close((uv_handle_t*)&pp->async, nullptr);
    uv_run(ploop, uv_run_mode::UV_RUN_DEFAULT);
    processes.erase(handle);
    currentHandle = 0;
}

void FChildProcessManager::OnUvProcessClosed(UVProcess_t* process, int64_t exit_status, int term_signal)
{
    //process->async.data = process;
    //uv_async_init(ploop, &process->async, [](uv_async_t* handle) {
    //    {
    //        UVProcess_t& UVProcess = *(UVProcess_t*)handle->data;

    //        UVProcess.ChildProcessManager->ClearProcessData(UVProcess.handle);
    //    }
    //    });
    //uv_async_send(&process->async);
    if (process->OnExitDelegate) {
        process->OnExitDelegate(process->handle, exit_status, term_signal);
    }
    process->ChildProcessManager->ClearProcessData(process->handle);
}

void FChildProcessManager::InternalSpawnProcess(UVProcess_t* pp)
{
    int r;
    UVProcess_t& UVProcess = *pp;
    auto& p = *pp;
    uv_pipe_init(ploop, &p.pipe, 0);
    p.pipe.data = pp;
    p.process.data = pp;
    p.options.stdio_count = 3;
    p.child_stdio[0].flags = UV_IGNORE;
    p.child_stdio[2].flags = UV_IGNORE;
    p.child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    p.child_stdio[1].data.stream = (uv_stream_t*)&p.pipe;
    p.options.stdio = p.child_stdio;
    p.options.file = pp->args[0];
    p.options.args = pp->args;
    p.options.flags = UV_PROCESS_WINDOWS_HIDE;
    p.options.exit_cb = [](uv_process_t* process, int64_t exit_status, int term_signal) {
        UVProcess_t& p = *(UVProcess_t*)process->data;
        p.ChildProcessManager->OnUvProcessClosed((UVProcess_t*)process->data, exit_status, term_signal);
    };

    if (r = uv_spawn(ploop, &p.process, &p.options)) {
        LOG_ERROR("{}", uv_strerror(r));
        return;
    }

    if ((r = uv_read_start((uv_stream_t*)&p.pipe, alloc_buffer, on_read))) {
        uv_close((uv_handle_t*)&pp->process, nullptr);
        LOG_ERROR("{}", uv_strerror(r));
        return;
    }
}
