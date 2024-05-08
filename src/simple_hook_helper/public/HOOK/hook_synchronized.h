#pragma once

#include <simple_os_defs.h>
#include <string>
#include "simple_hook_helper_common.h"

#define MUTEX_TEXTURE1 "CaptureHook_TextureMutex1"
#define MUTEX_TEXTURE2 "CaptureHook_TextureMutex2"

#define EVENT_CAPTURE_RESTART "CaptureHook_Restart"
#define EVENT_CAPTURE_STOP "CaptureHook_Stop"
#define EVENT_HOOK_READY "CaptureHook_HookReady"
#define EVENT_HOOK_EXIT "CaptureHook_Exit"
#define EVENT_HOOK_INIT "CaptureHook_Initialize"

#define WINDOW_HOOK_KEEPALIVE "CaptureHook_KeepAlive"

HOOK_HELPER_API HANDLE create_mutex_plus_id(const char* name, DWORD id, BOOL is_app);
HOOK_HELPER_API HANDLE open_mutex_plus_id(const char* name, DWORD id, BOOL is_app);
HOOK_HELPER_API HANDLE create_event_plus_id(const char* name, DWORD id, BOOL is_app);
HOOK_HELPER_API HANDLE open_event_plus_id(const char* name, DWORD id, BOOL is_app);

static inline std::string GetNamePlusID(std::string name, uint64_t id) {
   return  name+=std::to_string(id);
}