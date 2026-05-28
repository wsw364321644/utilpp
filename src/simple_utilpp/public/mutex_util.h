#pragma once
#include <system_error>
#include <handle.h>
#include <string>
#include <chrono>
#include "ipc_error.h"
#include "simple_export_ppdefs.h"
namespace utilpp {

    SIMPLE_UTIL_EXPORT const CommonHandlePtr_t CreateProcMutex(std::string_view name,std::error_code& ec);
    SIMPLE_UTIL_EXPORT const CommonHandlePtr_t OpenProcMutex(std::string_view name, std::error_code& ec);
    SIMPLE_UTIL_EXPORT bool TryLockProcMutex(const CommonHandlePtr_t,std::error_code& ec);
    SIMPLE_UTIL_EXPORT bool TryLockProcMutexInfinite(const CommonHandlePtr_t,std::error_code& ec);
    SIMPLE_UTIL_EXPORT bool LockProcMutex(const CommonHandlePtr_t, uint64_t microSec, std::error_code& ec);
    SIMPLE_UTIL_EXPORT bool ReleaseProcMutex(const CommonHandlePtr_t,std::error_code& ec);
    SIMPLE_UTIL_EXPORT bool CloseProcMutex(const CommonHandlePtr_t,std::error_code& ec);


    template <class _Rep, class _Period>
    bool LockProcMutex(const CommonHandlePtr_t handle, std::chrono::duration<_Rep, _Period> dur, std::error_code& ec) {
        std::chrono::milliseconds microdur = std::chrono::duration_cast<std::chrono::microseconds>(dur);
        LockProcMutex(handle, microdur.count(), ec);
    }
}