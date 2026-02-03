#pragma once
#include <handle.h>
#include <string>
#include <vector>
#include "ipc_util_export.h"

typedef struct SpawnData_t {
    std::string_view Filepath;
    std::vector<std::string_view> Argvs;
    bool bHideWindow{ false };
    bool bDetach{ false };
}SpawnData_t;
///use ChildProcessManager in one thread
class IChildProcessManager {
public:
    virtual CommonHandle32_t SpawnProcess(const char* filepath, const char** args = nullptr) = 0;
    virtual CommonHandle32_t SpawnProcess(SpawnData_t) = 0;
    typedef std::function< void(CommonHandle32_t, const char*, int64_t) > FOnReadDelegate;
    virtual void RegisterOnRead(CommonHandle32_t handle, FOnReadDelegate delegate) = 0;
    typedef std::function< void(CommonHandle32_t, int64_t, int) > FOnExitDelegate;
    virtual void RegisterOnExit(CommonHandle32_t handle, FOnExitDelegate delegate) = 0;
    virtual bool CheckIsFinished(CommonHandle32_t handle) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void Run() = 0;
    IPC_EXPORT static std::shared_ptr<IChildProcessManager> GetSingleton();
};

