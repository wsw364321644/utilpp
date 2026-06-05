#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits>
#include "simple_export_defs.h"
#include "simple_export_ppdefs.h"

SIMPLE_UTIL_API bool util_dll_path(char* path, size_t* size);
SIMPLE_UTIL_API bool util_exe_path(char* path, size_t* size);
SIMPLE_UTIL_API bool util_exe_wpath(wchar_t* path, size_t* size);
SIMPLE_UTIL_API bool util_dll_wpath(wchar_t* path, size_t* size);
SIMPLE_UTIL_API void* add_module_dir(const char* path, size_t size);
SIMPLE_UTIL_API void* add_module_wdir(const wchar_t* path, size_t size);
SIMPLE_UTIL_API void remove_module_dir(void* handle);

SIMPLE_UTIL_API typedef void (*util_thread_func)(void*);
SIMPLE_UTIL_API typedef void* util_thread_t;
SIMPLE_UTIL_API util_thread_t util_create_thread(void* arg, util_thread_func);
SIMPLE_UTIL_API void util_thread_join(util_thread_t  thread);

enum EBinaryType {
    FBT_NONE,
    FBT_MZ,
    FBT_PE,
    FBT_ELF
};
SIMPLE_UTIL_API EBinaryType get_binary_type(const char* path,int8_t* pointer_bytes);