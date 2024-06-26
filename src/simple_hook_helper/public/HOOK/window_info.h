#pragma once
#include <cstdint>
#define SHMEM_HOOK_WINDOW_INFO "CaptureHook_HookWindowInfo"

enum EHookWindowType
{
	Window,
	StatusBar,
	Background,
};

typedef struct hook_window_info_t {
	uint64_t shared_handle{ 0 };
	uint16_t max_height{ 0 };
	uint16_t max_width{ 0 };
	uint16_t min_height{ 0 };
	uint16_t min_width{ 0 };
	uint16_t height{ 0 };
	uint16_t width{ 0 };
	uint16_t x{ 0 };
	uint16_t y{ 0 };
	bool bNT_shared{ false };
	EHookWindowType hook_window_type{ EHookWindowType::Window };
}hook_window_info_t;