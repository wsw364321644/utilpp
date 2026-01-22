#include "mutex_util.h"
#include "ipc_error_internal.h"
#include <char_buffer_extension.h>
#include <simple_os_defs.h>
#include <unordered_map>
#include <filesystem>
typedef struct MutexInfoWin_t {
    HANDLE Handle{ NULL };
}MutexInfoWin_t;

namespace utilpp {
    const CommonHandlePtr_t CreateProcMutex(std::string_view name, std::error_code& ec) {
        FCharBuffer buf;
        MutexInfoWin_t info;
        const char* nameStr = GetStringViewCStr(name, buf);
        info.Handle = CreateMutexA(NULL, FALSE, nameStr);
        if (!info.Handle) {
            ec = make_ipc_error(EIPCError::IPCE_Unknow);
            return NullHandle;
        }
        DWORD dwRet = GetLastError();
        if (ERROR_ALREADY_EXISTS == dwRet) {
            ec = make_ipc_error(EIPCError::IPCE_AlreadyExist);
        }
        else {
            ec = make_ipc_error(EIPCError::IPCE_OK);
        }
        CommonHandlePtr_t HPtr((intptr_t)new MutexInfoWin_t(info));
        return HPtr;
    }

    const CommonHandlePtr_t OpenProcMutex(std::string_view name, std::error_code& ec)
    {
        FCharBuffer buf;
        MutexInfoWin_t info;
        const char* nameStr = GetStringViewCStr(name, buf);
        info.Handle = OpenMutexA(SYNCHRONIZE, FALSE, nameStr);
        if (!info.Handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return NullHandle;
        }
        ec = make_ipc_error(EIPCError::IPCE_OK);
        CommonHandlePtr_t HPtr((intptr_t)new MutexInfoWin_t(info));
        return HPtr;
    }

    bool TryLockProcMutex(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        return LockProcMutex(handle, 0, ec);
    }

    bool TryLockProcMutexInfinite(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        return LockProcMutex(handle, std::numeric_limits< uint64_t>::max(), ec);
    }

    bool LockProcMutex(const CommonHandlePtr_t handle, uint64_t microSec, std::error_code& ec)
    {
        if (!handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return false;
        }
        MutexInfoWin_t& info = *(MutexInfoWin_t*)handle.ID;
        auto dwWaitResult = WaitForSingleObject(info.Handle, DWORD(microSec / 1000));
        if (dwWaitResult != WAIT_OBJECT_0) {
            return false;
        }
        return true;
    }

    bool ReleaseProcMutex(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        if (!handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return false;
        }
        MutexInfoWin_t& info = *(MutexInfoWin_t*)handle.ID;
        return ReleaseMutex(info.Handle);
    }

    bool CloseProcMutex(const CommonHandlePtr_t handle, std::error_code& ec)
    {
        if (!handle) {
            ec = make_ipc_error(EIPCError::IPCE_NotExist);
            return false;
        }
        MutexInfoWin_t& info = *(MutexInfoWin_t*)handle.ID;
        auto bres = CloseHandle(info.Handle);
        if (bres) {
            delete (MutexInfoWin_t*)handle.ID;
            const_cast<CommonHandlePtr_t&>(handle).Reset();
        }
        return bres;
    }
}