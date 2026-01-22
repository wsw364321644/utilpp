#pragma once
#include <system_error>
#include <handle.h>
#include <string>
#include <chrono>
#include "ipc_error.h"
#include "ipc_util_export.h"
namespace utilpp {

    IPC_EXPORT const CommonHandlePtr_t CreateProcEvent(std::string_view name,std::error_code& ec);
    IPC_EXPORT const CommonHandlePtr_t OpenProcEvent(std::string_view name, std::error_code& ec);
    IPC_EXPORT bool SetProcEvent(const CommonHandlePtr_t, std::error_code& ec);
    IPC_EXPORT bool PeekProcEvent(const CommonHandlePtr_t, std::error_code& ec);
    IPC_EXPORT bool WaitForProcEventInfinite(const CommonHandlePtr_t, std::error_code& ec);
    IPC_EXPORT bool WaitForProcEvent(const CommonHandlePtr_t, uint64_t microSec, std::error_code& ec);
    IPC_EXPORT bool CloseProcEvent(const CommonHandlePtr_t, std::error_code& ec);


    template <class _Rep, class _Period>
    bool WaitForProcEvent(const CommonHandlePtr_t handle, std::chrono::duration<_Rep, _Period> dur, std::error_code& ec) {
        std::chrono::milliseconds microdur=std::chrono::duration_cast<std::chrono::microseconds>(dur);
        WaitForProcEvent(handle,microdur.count(),ec);
    }
}