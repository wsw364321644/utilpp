#pragma once
#include <handle.h>
#include "ipc_util_export.h"

IPC_API CommonHandle_t* CreateSharedMemory(const char* name, size_t len);
IPC_API CommonHandle_t* OpenSharedMemory(const char* name);
IPC_API void* MapSharedMemory(CommonHandle_t* phandle);
IPC_API void* MapReadSharedMemory(CommonHandle_t* phandle);
IPC_API void UnmapSharedMemory(void* ptr);
IPC_API bool WriteSharedMemory(CommonHandle_t* phandle, void* content, size_t);
IPC_API bool ReadSharedMemory(CommonHandle_t* phandle, void* content, size_t*);
IPC_API void CloseSharedMemory(CommonHandle_t* phandle);
