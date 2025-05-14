#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include "simple_export_defs.h"
#include "simple_export_ppdefs.h"

SIMPLE_UTIL_API void* simple_dlopen(const char* lib_name);
SIMPLE_UTIL_API void* simple_dlsym(void* handle, const char* func_name);
SIMPLE_UTIL_API bool simple_dlclose(void* handle);

SIMPLE_UTIL_EXPORT void* simple_dlopen(std::u8string_view lib_name);
SIMPLE_UTIL_EXPORT void* simple_dlsym(void* handle, std::string_view func_name);

