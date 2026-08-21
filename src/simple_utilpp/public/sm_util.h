#pragma once

#include "simple_export_ppdefs.h"
#include "simple_os_defs.h"
#include <handle.h>
#include <system_error>

SIMPLE_UTIL_EXPORT const CommonHandlePtr_t CreateSharedMemory(const char* name, size_t len, std::error_code& ec);
SIMPLE_UTIL_EXPORT const CommonHandlePtr_t OpenSharedMemory(const char* name, std::error_code& ec);
SIMPLE_UTIL_EXPORT void* MapSharedMemory(const CommonHandlePtr_t phandle);
SIMPLE_UTIL_EXPORT void* MapReadSharedMemory(const CommonHandlePtr_t phandle);
SIMPLE_UTIL_EXPORT void UnmapSharedMemory(const CommonHandlePtr_t,void* ptr);
SIMPLE_UTIL_EXPORT bool WriteSharedMemory(const CommonHandlePtr_t phandle, void* content, size_t, std::error_code& ec);
SIMPLE_UTIL_EXPORT bool ReadSharedMemory(const CommonHandlePtr_t phandle, void* content, size_t*, std::error_code& ec);
SIMPLE_UTIL_EXPORT void CloseSharedMemory(const CommonHandlePtr_t phandle);
SIMPLE_UTIL_EXPORT F_HANDLE SharedMemoryGetOSHandle(const CommonHandlePtr_t phandle);