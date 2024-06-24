#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "simple_export_defs.h"

/// DEPRECATED
SIMPLE_UTIL_API int util_dll_path(char* path, size_t* size);
/// DEPRECATED
SIMPLE_UTIL_API int util_exe_path(char* path, size_t* size);
SIMPLE_UTIL_API int util_exe_wpath(wchar_t* path, size_t* size);
SIMPLE_UTIL_API int util_dll_wpath(wchar_t* path, size_t* size);
SIMPLE_UTIL_API uint64_t GetProcessParentId(uint64_t* id);
SIMPLE_UTIL_API bool GetProcessIdFromHandle(void* handle, uint64_t* id);
SIMPLE_UTIL_API bool GetProcessIdFromPipe(void* handle, uint64_t* id);

SIMPLE_UTIL_API typedef void (*util_thread_func)(void*);
SIMPLE_UTIL_API typedef void* util_thread_t;
SIMPLE_UTIL_API util_thread_t util_create_thread(void* arg, util_thread_func);
SIMPLE_UTIL_API void util_thread_join(util_thread_t  thread);

