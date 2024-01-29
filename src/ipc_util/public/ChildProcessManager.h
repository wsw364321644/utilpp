#pragma once
#include <handle.h>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <functional>
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
# define SSIZE_MAX INTPTR_MAX
# define _SSIZE_T_
# define _SSIZE_T_DEFINED
#else
#include <unistd.h>
#endif
#endif
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_pipe_s uv_pipe_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_buf_t uv_buf_t;


typedef struct UVProcess_t UVProcess_t;
///use FChildProcessManager in one thread
class FChildProcessManager {
public:
    FChildProcessManager();
    ~FChildProcessManager() {}
    static FChildProcessManager* GetInstance();
    CommonHandle_t SpawnProcess(const char* filepath,const char** args=nullptr);
    typedef std::function< void (CommonHandle_t,const char* , SSIZE_T) > FOnReadDelegate;
    void RegisterOnRead(CommonHandle_t handle, FOnReadDelegate delegate);
    typedef std::function< void(CommonHandle_t,int64_t, int) > FOnExitDelegate;
    void RegisterOnExit(CommonHandle_t handle, FOnExitDelegate delegate);
    bool CheckIsFinished(CommonHandle_t handle);
    void Tick(float delSec);
    void Run();
    void ClearProcessData(CommonHandle_t handle);
    void OnUvProcessClosed(UVProcess_t* process, int64_t exit_status, int term_signal);
private :
    void InternalSpawnProcess(UVProcess_t*);
    std::atomic_uint32_t processCount{0};
    uv_loop_t* ploop;
    CommonHandle_t currentHandle{ 0 };
    std::unordered_map<CommonHandle_t, std::shared_ptr<UVProcess_t>> processes;
};
