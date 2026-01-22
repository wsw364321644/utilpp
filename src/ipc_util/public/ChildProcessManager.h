#pragma once
#include <handle.h>
#include "ipc_util_export.h"

typedef struct UVProcess_t UVProcess_t;
///use ChildProcessManager in one thread
class IChildProcessManager {
public:
    virtual CommonHandle32_t SpawnProcess(const char* filepath, const char** args = nullptr) = 0;
    typedef std::function< void(CommonHandle32_t, const char*, int64_t) > FOnReadDelegate;
    virtual void RegisterOnRead(CommonHandle32_t handle, FOnReadDelegate delegate) = 0;
    typedef std::function< void(CommonHandle32_t, int64_t, int) > FOnExitDelegate;
    virtual void RegisterOnExit(CommonHandle32_t handle, FOnExitDelegate delegate) = 0;
    virtual bool CheckIsFinished(CommonHandle32_t handle) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void Run() = 0;
    IPC_EXPORT static std::shared_ptr<IChildProcessManager> GetSingleton();
};

