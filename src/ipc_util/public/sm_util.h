#pragma once
#include <handle.h>
#include "ipc_util_export.h"

IPC_EXPORT const CommonHandlePtr_t CreateSharedMemory(const char* name, size_t len);
IPC_EXPORT const CommonHandlePtr_t OpenSharedMemory(const char* name);
IPC_EXPORT void* MapSharedMemory(const CommonHandlePtr_t phandle);
IPC_EXPORT void* MapReadSharedMemory(const CommonHandlePtr_t phandle);
IPC_EXPORT void UnmapSharedMemory(const CommonHandlePtr_t,void* ptr);
IPC_EXPORT bool WriteSharedMemory(const CommonHandlePtr_t phandle, void* content, size_t);
IPC_EXPORT bool ReadSharedMemory(const CommonHandlePtr_t phandle, void* content, size_t*);
IPC_EXPORT void CloseSharedMemory(const CommonHandlePtr_t phandle);
