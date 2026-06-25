#include "ChildProcessManager.h"
#include <FunctionExitHelper.h>
#include <singleton.h>
#include <LoggerHelper.h>
#include <CharBuffer.h>
#include <uv.h>
#include <cstring>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <functional>
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_pipe_s uv_pipe_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_buf_t uv_buf_t;
typedef struct UVProcess_t UVProcess_t;

class FChildProcessManager :public IChildProcessManager {
public:
    FChildProcessManager();
    ~FChildProcessManager() {}
    static FChildProcessManager* GetInstance();
    CommonHandle32_t SpawnProcess(const char* filepath, const char** args = nullptr) override;
    CommonHandle32_t SpawnProcess(SpawnData_t) override;
    pid_t SpawnDetachProcess(SpawnData_t)override;
    typedef std::function< void(CommonHandle32_t, const char*, int64_t) > FOnReadDelegate;
    void RegisterOnRead(CommonHandle32_t handle, FOnReadDelegate delegate)override;
    typedef std::function< void(CommonHandle32_t, int64_t, int) > FOnExitDelegate;
    void RegisterOnExit(CommonHandle32_t handle, FOnExitDelegate delegate)override;
    bool CheckIsFinished(CommonHandle32_t handle);
    void Tick(float delSec)override;
    void Run()override;

private:
    void ClearProcessData(CommonHandle32_t handle);
    void OnUvProcessClosed(UVProcess_t* process, int64_t exit_status, int term_signal);
    bool InternalSpawnProcess(UVProcess_t*);
    std::atomic_uint32_t processCount{ 0 };
    uv_loop_t* ploop;
    CommonHandle32_t currentHandle{ NullHandle };
    std::unordered_map<CommonHandle32_t, std::shared_ptr<UVProcess_t>> processes;
};

typedef struct UVProcess_t {
    ~UVProcess_t() {
        if (args) {
            for (int i = 0; args[i] != 0; i++) {
                free(args[i]);
            }
            free(args);
            args = nullptr;
        }
        if (cwd) {
            free(cwd);
            cwd = nullptr;
        }
    }
    bool AllocCDW(size_t s) {
        cwd = (char*)calloc(s, sizeof(char));
        return cwd != nullptr;
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
    char* cwd{ nullptr };
    uv_process_t process{ 0 };
    uv_process_options_t options{ 0 };
    uv_stdio_container_t child_stdio[4]{};
    uv_pipe_t in_pipe{};
    uv_pipe_t out_pipe{};
    uv_pipe_t err_pipe{};
    uv_async_t async{};
    FChildProcessManager* ChildProcessManager{ NULL };
    FChildProcessManager::FOnReadDelegate  OnReadDelegate;
    FChildProcessManager::FOnExitDelegate  OnExitDelegate;
    CommonHandle32_t handle{ NullHandle };
    FCharBuffer Buf;
    bool bHideWindow{ true };
    bool bDetach{ false };
}UVProcess_t;
void alloc_buffer(uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf) {
    UVProcess_t& p = *(UVProcess_t*)handle->data;
    p.Buf.Reverse(suggested_size);
    *buf = uv_buf_init((char*)p.Buf.Data(), p.Buf.Capacity());
};
void on_read(uv_stream_t* stream,
    ssize_t nread,
    const uv_buf_t* buf) {
    UVProcess_t& p = *(UVProcess_t*)stream->data;

    if (nread < 0) {
        if (nread != uv_errno_t::UV_EOF) {
            SIMPLELOG_LOGGER_ERROR(nullptr, "{}", uv_strerror(nread));
        }
    }
    else {
        if (p.OnReadDelegate) {
            p.OnReadDelegate(p.handle, buf->base, nread);
        }
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
CommonHandle32_t FChildProcessManager::SpawnProcess(const char* _filepath, const char** _args)
{
    SpawnData_t spawnData;
    spawnData.Filepath = _filepath;
    if (_args) {
        for (int i = 0; _args[i] != nullptr; i++) {
            ++spawnData.Argc;
        }
        spawnData.Argvs = new std::string_view[spawnData.Argc];
        for (int i = 0; _args[i] != nullptr; i++) {
            spawnData.Argvs[i] = _args[i];
        }
    }
    FunctionExitHelper_t deleteArgvs(
        [&]() {
            if (spawnData.Argvs) {
                if (_args) {
                    delete[] spawnData.Argvs;
                }
                spawnData.Argvs = nullptr;
            }
        }
    );
    return SpawnProcess(spawnData);
}

CommonHandle32_t FChildProcessManager::SpawnProcess(SpawnData_t spawnData)
{
    if (spawnData.Filepath.empty()) {
        return NullHandle;
    }
    bool bres = false;
    auto pair = processes.try_emplace(processCount, std::make_shared< UVProcess_t>());
    if (!pair.second) {
        return NullHandle;
    }
    FunctionExitHelper_t eraseProcess(
        [&]() {
            if (!bres) {
                processes.erase(pair.first);
            }
        }
    );
    UVProcess_t* pp = pair.first->second.get();
    pp->ChildProcessManager = this;
    pp->handle = pair.first->first;
    int argc = 1 + spawnData.Argc;

    if (!pp->AllocArgs(argc)) {
        return NullHandle;
    }
    if (!pp->AllocArgsIndex(0, spawnData.Filepath.size() + 1)) {
        return NullHandle;
    }
    memcpy(pp->args[0], spawnData.Filepath.data(), spawnData.Filepath.size());
    pp->args[0][spawnData.Filepath.size()] = '\0';
    for (int i = 0; argc - i > 1; i++) {
        auto& argv = spawnData.Argvs[i];
        pp->AllocArgsIndex(i + 1, argv.size() + 1);
        memcpy(pp->args[i + 1], argv.data(), argv.size());
        pp->args[i + 1][argv.size()] = '\0';
    }
    pp->bHideWindow = spawnData.bHideWindow;
    if (!spawnData.CWD.empty()) {
        if (!pp->AllocCDW(spawnData.CWD.size() + 1)) {
            return NullHandle;
        }
        memcpy(pp->cwd, spawnData.CWD.data(), spawnData.CWD.size());
    }
    bres = true;
    return pair.first->first;
}

pid_t FChildProcessManager::SpawnDetachProcess(SpawnData_t spawnData)
{
    int r;
    UVProcess_t p;
    if (!p.AllocArgs(spawnData.Argc + 1)) {
        return NULL;
    }
    if (!p.AllocArgsIndex(0, spawnData.Filepath.size() + 1)) {
        return NULL;
    }
    memcpy(p.args[0], spawnData.Filepath.data(), spawnData.Filepath.size());
    p.args[0][spawnData.Filepath.size()] = '\0';
    for (int i = 0; i < spawnData.Argc; i++) {
        auto& argv = spawnData.Argvs[i];
        p.AllocArgsIndex(i + 1, argv.size() + 1);
        memcpy(p.args[i + 1], argv.data(), argv.size());
        p.args[i + 1][argv.size()] = '\0';
    }

    int flags = 0;
    if (spawnData.bHideWindow) {
        flags |= UV_PROCESS_WINDOWS_HIDE;
    }
    flags |= UV_PROCESS_DETACHED;

    p.options.stdio_count = 3;
    p.child_stdio[0].flags = UV_IGNORE;
    p.child_stdio[1].flags = UV_IGNORE;
    p.child_stdio[2].flags = UV_IGNORE;

    p.options.stdio = p.child_stdio;
    p.options.file = p.args[0];
    p.options.args = p.args;
    p.options.flags = flags;
    if (!spawnData.CWD.empty()) {
        p.options.cwd = spawnData.CWD.data();
    }
    if (r = uv_spawn(ploop, &p.process, &p.options)) {
        if (r != uv_errno_t::UV_ENOENT) {
            SIMPLELOG_LOGGER_ERROR(nullptr, "{}", uv_strerror(r));
        }
        return NULL;
    }
    auto pid = uv_process_get_pid(&p.process);
    uv_unref((uv_handle_t*)&p.process);
    return pid;
}

void FChildProcessManager::RegisterOnRead(CommonHandle32_t handle, FOnReadDelegate delegate)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    pair->second->OnReadDelegate = delegate;
}
void FChildProcessManager::RegisterOnExit(CommonHandle32_t handle, FOnExitDelegate delegate)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    pair->second->OnExitDelegate = delegate;
}
bool FChildProcessManager::CheckIsFinished(CommonHandle32_t handle)
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
        if (!InternalSpawnProcess(itr->second.get())) {
            itr->second->OnExitDelegate(itr->first, -1, 0);
            processes.erase(itr);
        }
    }
    uv_run(ploop, uv_run_mode::UV_RUN_NOWAIT);
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
        if (!InternalSpawnProcess(itr->second.get())) {
            itr->second->OnExitDelegate(itr->first, -1, 0);
            processes.erase(itr);
        }
    } while (currentHandle.IsValid());
}

void FChildProcessManager::ClearProcessData(CommonHandle32_t handle)
{
    auto pair = processes.find(handle);
    if (pair == processes.end()) {
        return;
    }
    auto pp = pair->second.get();
    uv_close((uv_handle_t*)&pp->process, nullptr);
    uv_close((uv_handle_t*)&pp->in_pipe, nullptr);
    uv_close((uv_handle_t*)&pp->out_pipe, nullptr);
    uv_close((uv_handle_t*)&pp->err_pipe, nullptr);
    //uv_close((uv_handle_t*)&pp->async, nullptr);
    uv_run(ploop, uv_run_mode::UV_RUN_DEFAULT);
    processes.erase(handle);
    currentHandle = NullHandle;
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

bool FChildProcessManager::InternalSpawnProcess(UVProcess_t* pp)
{
    int r;
    UVProcess_t& UVProcess = *pp;
    auto& p = *pp;
    int flags = 0;
    if (p.bHideWindow) {
        flags |= UV_PROCESS_WINDOWS_HIDE;
    }

    uv_pipe_init(ploop, &p.in_pipe, 0);
    p.in_pipe.data = pp;
    uv_pipe_init(ploop, &p.out_pipe, 0);
    p.out_pipe.data = pp;
    uv_pipe_init(ploop, &p.err_pipe, 0);
    p.err_pipe.data = pp;
    p.process.data = pp;
    p.options.stdio_count = 3;

    p.child_stdio[0].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE);
    p.child_stdio[0].data.stream = (uv_stream_t*)&p.in_pipe;
    p.child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    p.child_stdio[1].data.stream = (uv_stream_t*)&p.out_pipe;
    p.child_stdio[2].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    p.child_stdio[2].data.stream = (uv_stream_t*)&p.err_pipe;

    p.options.stdio = p.child_stdio;
    p.options.file = pp->args[0];
    p.options.args = pp->args;
    p.options.flags = flags;
    p.options.exit_cb = [](uv_process_t* process, int64_t exit_status, int term_signal) {
        UVProcess_t& p = *(UVProcess_t*)process->data;
        p.ChildProcessManager->OnUvProcessClosed((UVProcess_t*)process->data, exit_status, term_signal);
        };
    if (pp->cwd) {
        p.options.cwd = pp->cwd;
    }
    if (r = uv_spawn(ploop, &p.process, &p.options)) {
        if (r != uv_errno_t::UV_ENOENT) {
            SIMPLELOG_LOGGER_ERROR(nullptr, "{}", uv_strerror(r));
        }
        return false;
    }

    if ((r = uv_read_start((uv_stream_t*)&p.out_pipe, alloc_buffer, on_read))) {
        uv_close((uv_handle_t*)&pp->process, nullptr);
        SIMPLELOG_LOGGER_ERROR(nullptr, "{}", uv_strerror(r));
        return false;
    }
    if ((r = uv_read_start((uv_stream_t*)&p.err_pipe, alloc_buffer, on_read))) {
        uv_close((uv_handle_t*)&pp->process, nullptr);
        SIMPLELOG_LOGGER_ERROR(nullptr, "{}", uv_strerror(r));
        return false;
    }
    return true;
}

std::shared_ptr<IChildProcessManager> IChildProcessManager::GetSingleton()
{
    return TClassSingletonHelper<FChildProcessManager>::GetClassSingleton();
}