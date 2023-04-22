#pragma once
#include <handle.h>

CommonHandle_t OpenSharedMemory(const char* name);
bool WriteSharedMemory(CommonHandle_t handle, void* content, size_t*);
bool ReadSharedMemory(CommonHandle_t handle, void* content, size_t*);
void CloseSharedMemory(CommonHandle_t handle);
