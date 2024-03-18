/**
 *  util.h
 */

#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#if defined(__cplusplus) && !defined(_M_ARM)
extern "C" {
#endif
    /// DEPRECATED
    int util_dll_path(char* path, size_t* size);
    /// DEPRECATED
    int util_exe_path(char* path, size_t* size);
    int util_exe_wpath(wchar_t* path, size_t* size);
    int util_dll_wpath(wchar_t* path, size_t* size);
    uint64_t GetProcessParentId(uint64_t* id);
    bool GetProcessIdFromHandle(void* handle, uint64_t* id);
    bool GetProcessIdFromPipe(void* handle, uint64_t* id);

    typedef void (*util_thread_func)(void*);
    typedef void* util_thread_t;
    util_thread_t util_create_thread(void* arg, util_thread_func);
    void util_thread_join(util_thread_t  thread);

#if defined(__cplusplus) && !defined(_M_ARM)
}
#endif
