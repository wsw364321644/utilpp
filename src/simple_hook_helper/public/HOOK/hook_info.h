#pragma once
#include "graphics_info.h"

#define HOOK_VER_MAJOR 0
#define HOOK_VER_MINOR 1
#define HOOK_VER_PATCH 0

#define SHMEM_HOOK_INFO "CaptureHook_HookInfo"
#define OVERLAY_HOT_KEY_NAME "CaptureHook_HotKey"

typedef struct hook_info_t {
	uint32_t hook_ver_major;
	uint32_t hook_ver_minor;

	/* capture info */
	enum ECaptureType type;
	uint32_t window;
	uint32_t format;
	uint32_t cx;
	uint32_t cy;
	uint32_t UNUSED_base_cx;
	uint32_t UNUSED_base_cy;
	uint32_t pitch;
	uint32_t map_id;
	uint32_t map_size;
	bool flip;
	bool bOverlayEnabled ;
	
	/* additional options */
	uint64_t frame_interval;
	bool UNUSED_use_scale;
	bool force_shmem;
	bool capture_overlay;
	bool allow_srgb_alias;
	graphics_offsets_t offsets;
}hook_info_t;

