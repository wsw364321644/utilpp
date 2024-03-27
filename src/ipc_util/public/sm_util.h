#pragma once
#include <handle.h>

CommonHandle_t* CreateSharedMemory(const char* name);
CommonHandle_t* OpenSharedMemory(const char* name);
void* MapSharedMemory(CommonHandle_t* phandle);
void* MapReadSharedMemory(CommonHandle_t* phandle);
void UnmapSharedMemory(void* ptr);
bool WriteSharedMemory(CommonHandle_t* phandle, void* content, size_t*);
bool ReadSharedMemory(CommonHandle_t* phandle, void* content, size_t*);
void CloseSharedMemory(CommonHandle_t* phandle);
