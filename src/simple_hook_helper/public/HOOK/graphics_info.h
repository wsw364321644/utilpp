#pragma once
#include <cstdint>


#define SHMEM_GRAPHIC_INFO "CaptureHook_GraphicInfo"
#define SHMEM_TEXTURE  "CaptureHook_Texture"

#define NUM_BUFFERS 3

typedef struct graphic_info_t{
	uint32_t Width;
	uint32_t Height;
	uint32_t Pitch;
}graphic_info_t;

typedef struct d3d8_offsets_t {
	uint32_t present;
}d3d8_offsets_t;

typedef struct d3d9_offsets_t {
	uint32_t present;
	uint32_t present_ex;
	uint32_t present_swap;
	uint32_t d3d9_clsoff;
	uint32_t is_d3d9ex_clsoff;
}d3d9_offsets_t;

typedef struct d3d12_offsets_t {
	uint32_t execute_command_lists;
}d3d12_offsets_t;

typedef struct dxgi_offsets_t {
	uint32_t present;
	uint32_t resize;
	uint32_t present1;
}dxgi_offsets_t;

typedef struct dxgi_offsets2_t {
	uint32_t release;
}dxgi_offsets2_t;

typedef struct ddraw_offsets_t {
	uint32_t surface_create;
	uint32_t surface_restore;
	uint32_t surface_release;
	uint32_t surface_unlock;
	uint32_t surface_blt;
	uint32_t surface_flip;
	uint32_t surface_set_palette;
	uint32_t palette_set_entries;
}ddraw_offsets_t;

typedef struct graphics_offsets_t {
	struct d3d8_offsets_t d3d8;
	struct d3d9_offsets_t d3d9;
	struct dxgi_offsets_t dxgi;
	struct ddraw_offsets_t ddraw;
	struct dxgi_offsets2_t dxgi2;
	struct d3d12_offsets_t d3d12;
}graphics_offsets_t;

enum ECaptureType {
	CAPTURE_TYPE_MEMORY,
	CAPTURE_TYPE_TEXTURE,
};

typedef struct shmem_data_t {
	volatile int last_tex;
	uint32_t tex1_offset;
	uint32_t tex2_offset;
}shmem_data_t;

typedef struct shtex_data_t {
	uint32_t tex_handle;
}shtex_data_t;
