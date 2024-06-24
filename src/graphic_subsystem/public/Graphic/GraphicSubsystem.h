///about sharing resource https://learn.microsoft.com/en-us/windows/win32/direct3darticles/surface-sharing-between-windows-graphics-apis

#pragma once
#include <cstdint>
#include <bitset>
#include <std_ext.h>
#include "graphic_subsystem_exports.h"

enum class EGraphicSubsystemColorFormat:uint16_t {
	UNKNOWN,
	A8,
	R8,
	RGBA,
	BGRX,
	BGRA,
	R10G10B10A2,
	RGBA16,
	R16,
	RGBA16F,
	RGBA32F,
	RG16F,
	RG32F,
	R16F,
	R32F,
	DXT1,
	DXT3,
	DXT5,
	R8G8,
	RGBA_UNORM,
	BGRX_UNORM,
	BGRA_UNORM,
	RG16,
};
enum class EGraphicSubsystemError
{
	OK,
	UNKNOWN,
	DXGIFailed,
};

enum class EGraphicSubsystem
{
	OPENGL,
	DX9,
	DX11,
	DX12,
};

enum class EGraphicSubsystemTextureType
{
	TEXTURE_1D,
	TEXTURE_1D_ARRAY,
	TEXTURE_2D,
	TEXTURE_2D_ARRAY,
	TEXTURE_3D,
};

typedef enum ETextureFlag
{
	BIND_VERTEX_BUFFER,
	BIND_INDEX_BUFFER,
	BIND_CONSTANT_BUFFE,
	BIND_SHADER_RESOURCE,
	BIND_STREAM_OUTPUT,
	BIND_RENDER_TARGET,
	BIND_DEPTH_STENCIL,
	BIND_UNORDERED_ACCESS,
	BIND_DECODER,
	BIND_VIDEO_ENCODER,
	MISC_GENERATE_MIPS,
	MISC_SHARED,
	MISC_TEXTURECUBE,
	MISC_DRAWINDIRECT_ARGS,
	MISC_BUFFER_ALLOW_RAW_VIEWS,
	MISC_BUFFER_STRUCTURED,
	MISC_RESOURCE_CLAMP,
	MISC_SHARED_KEYEDMUTEX,
	MISC_GDI_COMPATIBLE,
	MISC_SHARED_NTHANDLE,
	MISC_RESTRICTED_CONTENT,
	MISC_RESTRICT_SHARED_RESOURCE,
	MISC_RESTRICT_SHARED_RESOURCE_DRIVER,
	MISC_GUARDED,
	MISC_TILE_POOL,
	MISC_TILED,
	MISC_HW_PROTECTED,
	MISC_SHARED_DISPLAYABLE,
	MISC_SHARED_EXCLUSIVE_WRITER,
	TWO_PLANE,
	CPU_ACCESS_WRITE,
	CPU_ACCESS_READ,
	COUNT
}ETextureFlag;
typedef std::bitset<std::to_underlying(ETextureFlag::COUNT)> TextureFlag_t;

class FGraphicSubsystemDevice;

class FGraphicSubsystemObj {
public:
	FGraphicSubsystemObj(FGraphicSubsystemDevice* device) :Device(device) {}
	virtual ~FGraphicSubsystemObj() = default;
protected:
	FGraphicSubsystemDevice* Device;
};

class FGraphicSubsystemTexture :public FGraphicSubsystemObj {
public:
	FGraphicSubsystemTexture(FGraphicSubsystemDevice* device) :FGraphicSubsystemObj(device) {}
	virtual ~FGraphicSubsystemTexture() = default;
	virtual uint32_t GetWidth() const = 0;
	virtual	uint32_t GetHeight() const = 0;
	virtual	uint32_t GetByteSize() const = 0;
	virtual	EGraphicSubsystemTextureType GetType() const = 0;
	virtual	EGraphicSubsystemColorFormat GetColorFormat() const = 0;
	virtual bool IsShared() const = 0;
	virtual uint64_t GetSharedHandle() const = 0;
protected:
	bool bAcquired{false};
};

class FGraphicSubsystemDevice :public FGraphicSubsystemObj {
public:
	FGraphicSubsystemDevice() :FGraphicSubsystemObj(this) {}
	virtual ~FGraphicSubsystemDevice() = default;
};


class FGraphicSubsystem {
public:
	virtual const char* DeviceGetName(void) = 0;
	virtual EGraphicSubsystem DeviceGetType(void)=0;
	virtual EGraphicSubsystemError DeviceEnumAdapters(bool (*callback)(void* param, const char* name,uint32_t id),void* param)=0;
	//virtual const char* device_preprocessor_name(void);
	virtual EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter)=0;
	virtual void DeviceDestroy(FGraphicSubsystemDevice* device)=0;
//	void (*device_enter_context)(gs_device_t* device);
//	void (*device_leave_context)(gs_device_t* device);
//	void* (*device_get_device_obj)(gs_device_t* device);
//	gs_swapchain_t* (*device_swapchain_create)(
//		gs_device_t* device, const struct gs_init_data* data);
//	void (*device_resize)(gs_device_t* device, uint32_t x, uint32_t y);
//	enum gs_color_space(*device_get_color_space)(gs_device_t* device);
//	void (*device_update_color_space)(gs_device_t* device);
//	void (*device_get_size)(const gs_device_t* device, uint32_t* x,
//		uint32_t* y);
//	uint32_t(*device_get_width)(const gs_device_t* device);
//	uint32_t(*device_get_height)(const gs_device_t* device);

	virtual FGraphicSubsystemTexture* DeviceTextureCreate(
		FGraphicSubsystemDevice* device, uint32_t width, uint32_t height,
		enum EGraphicSubsystemColorFormat color_format, uint32_t levels,
		const uint8_t** data, TextureFlag_t flags)=0;
//	gs_texture_t* (*device_cubetexture_create)(
//		gs_device_t* device, uint32_t size,
//		enum gs_color_format color_format, uint32_t levels,
//		const uint8_t** data, uint32_t flags);
//	gs_texture_t* (*device_voltexture_create)(
//		gs_device_t* device, uint32_t width, uint32_t height,
//		uint32_t depth, enum gs_color_format color_format,
//		uint32_t levels, const uint8_t* const* data, uint32_t flags);
//	gs_zstencil_t* (*device_zstencil_create)(
//		gs_device_t* device, uint32_t width, uint32_t height,
//		enum gs_zstencil_format format);
//	gs_stagesurf_t* (*device_stagesurface_create)(
//		gs_device_t* device, uint32_t width, uint32_t height,
//		enum gs_color_format color_format);
//	gs_samplerstate_t* (*device_samplerstate_create)(
//		gs_device_t* device, const struct gs_sampler_info* info);
//	gs_shader_t* (*device_vertexshader_create)(gs_device_t* device,
//		const char* shader,
//		const char* file,
//		char** error_string);
//	gs_shader_t* (*device_pixelshader_create)(gs_device_t* device,
//		const char* shader,
//		const char* file,
//		char** error_string);
//	gs_vertbuffer_t* (*device_vertexbuffer_create)(gs_device_t* device,
//		struct gs_vb_data* data,
//		uint32_t flags);
//	gs_indexbuffer_t* (*device_indexbuffer_create)(gs_device_t* device,
//		enum gs_index_type type,
//		void* indices,
//		size_t num,
//		uint32_t flags);
//	gs_timer_t* (*device_timer_create)(gs_device_t* device);
//	gs_timer_range_t* (*device_timer_range_create)(gs_device_t* device);
//	enum gs_texture_type(*device_get_texture_type)(
//		const gs_texture_t* texture);
//	void (*device_load_vertexbuffer)(gs_device_t* device,
//		gs_vertbuffer_t* vertbuffer);
//	void (*device_load_indexbuffer)(gs_device_t* device,
//		gs_indexbuffer_t* indexbuffer);
//	void (*device_load_texture)(gs_device_t* device, gs_texture_t* tex,
//		int unit);
//	void (*device_load_samplerstate)(gs_device_t* device,
//		gs_samplerstate_t* samplerstate,
//		int unit);
//	void (*device_load_vertexshader)(gs_device_t* device,
//		gs_shader_t* vertshader);
//	void (*device_load_pixelshader)(gs_device_t* device,
//		gs_shader_t* pixelshader);
//	void (*device_load_default_samplerstate)(gs_device_t* device, bool b_3d,
//		int unit);
//	gs_shader_t* (*device_get_vertex_shader)(const gs_device_t* device);
//	gs_shader_t* (*device_get_pixel_shader)(const gs_device_t* device);
//	gs_texture_t* (*device_get_render_target)(const gs_device_t* device);
//	gs_zstencil_t* (*device_get_zstencil_target)(const gs_device_t* device);
//	void (*device_set_render_target)(gs_device_t* device, gs_texture_t* tex,
//		gs_zstencil_t* zstencil);
//	void (*device_set_render_target_with_color_space)(
//		gs_device_t* device, gs_texture_t* tex, gs_zstencil_t* zstencil,
//		enum gs_color_space space);
//	void (*device_set_cube_render_target)(gs_device_t* device,
//		gs_texture_t* cubetex, int side,
//		gs_zstencil_t* zstencil);
//	void (*device_enable_framebuffer_srgb)(gs_device_t* device,
//		bool enable);
//	bool (*device_framebuffer_srgb_enabled)(gs_device_t* device);
	virtual void DeviceCopyTexture(FGraphicSubsystemDevice* device, FGraphicSubsystemTexture* dst, FGraphicSubsystemTexture* src) {
		DeviceCopyTextureRegion(device, dst, 0, 0, src, 0, 0, 0, 0);
	}
	virtual void DeviceCopyTextureRegion(FGraphicSubsystemDevice* device,FGraphicSubsystemTexture* dst, uint32_t dst_x,uint32_t dst_y, 
		FGraphicSubsystemTexture* src,uint32_t src_x, uint32_t src_y,uint32_t src_w, uint32_t src_h) =0;
//	void (*device_stage_texture)(gs_device_t* device, gs_stagesurf_t* dst,
//		gs_texture_t* src);
//	void (*device_begin_frame)(gs_device_t* device);
//	void (*device_begin_scene)(gs_device_t* device);
//	void (*device_draw)(gs_device_t* device, enum gs_draw_mode draw_mode,
//		uint32_t start_vert, uint32_t num_verts);
//	void (*device_end_scene)(gs_device_t* device);
//	void (*device_load_swapchain)(gs_device_t* device,
//		gs_swapchain_t* swaphchain);
//	void (*device_clear)(gs_device_t* device, uint32_t clear_flags,
//		const struct vec4* color, float depth,
//		uint8_t stencil);
//	bool (*device_is_present_ready)(gs_device_t* device);
//	void (*device_present)(gs_device_t* device);
//	void (*device_flush)(gs_device_t* device);
//	void (*device_set_cull_mode)(gs_device_t* device,
//		enum gs_cull_mode mode);
//	enum gs_cull_mode(*device_get_cull_mode)(const gs_device_t* device);
//	void (*device_enable_blending)(gs_device_t* device, bool enable);
//	void (*device_enable_depth_test)(gs_device_t* device, bool enable);
//	void (*device_enable_stencil_test)(gs_device_t* device, bool enable);
//	void (*device_enable_stencil_write)(gs_device_t* device, bool enable);
//	void (*device_enable_color)(gs_device_t* device, bool red, bool green,
//		bool blue, bool alpha);
//	void (*device_blend_function)(gs_device_t* device,
//		enum gs_blend_type src,
//		enum gs_blend_type dest);
//	void (*device_blend_function_separate)(gs_device_t* device,
//		enum gs_blend_type src_c,
//		enum gs_blend_type dest_c,
//		enum gs_blend_type src_a,
//		enum gs_blend_type dest_a);
//	void (*device_blend_op)(gs_device_t* device, enum gs_blend_op_type op);
//
//	void (*device_depth_function)(gs_device_t* device,
//		enum gs_depth_test test);
//	void (*device_stencil_function)(gs_device_t* device,
//		enum gs_stencil_side side,
//		enum gs_depth_test test);
//	void (*device_stencil_op)(gs_device_t* device,
//		enum gs_stencil_side side,
//		enum gs_stencil_op_type fail,
//		enum gs_stencil_op_type zfail,
//		enum gs_stencil_op_type zpass);
//	void (*device_set_viewport)(gs_device_t* device, int x, int y,
//		int width, int height);
//	void (*device_get_viewport)(const gs_device_t* device,
//		struct gs_rect* rect);
//	void (*device_set_scissor_rect)(gs_device_t* device,
//		const struct gs_rect* rect);
//	void (*device_ortho)(gs_device_t* device, float left, float right,
//		float top, float bottom, float znear, float zfar);
//	void (*device_frustum)(gs_device_t* device, float left, float right,
//		float top, float bottom, float znear,
//		float zfar);
//	void (*device_projection_push)(gs_device_t* device);
//	void (*device_projection_pop)(gs_device_t* device);
//
//	void (*gs_swapchain_destroy)(gs_swapchain_t* swapchain);
//
//	void (*gs_texture_destroy)(gs_texture_t* tex);
//	uint32_t(*gs_texture_get_width)(const gs_texture_t* tex);
//	uint32_t(*gs_texture_get_height)(const gs_texture_t* tex);
//	enum gs_color_format(*gs_texture_get_color_format)(
//		const gs_texture_t* tex);
virtual bool TextureMap(FGraphicSubsystemTexture* tex, uint8_t** ptr, uint32_t* linesize) = 0;
virtual bool TextureUnmap(FGraphicSubsystemTexture* tex) = 0;

//	bool (*gs_texture_is_rect)(const gs_texture_t* tex);
//	void* (*gs_texture_get_obj)(const gs_texture_t* tex);
//
//	void (*gs_cubetexture_destroy)(gs_texture_t* cubetex);
//	uint32_t(*gs_cubetexture_get_size)(const gs_texture_t* cubetex);
//	enum gs_color_format(*gs_cubetexture_get_color_format)(
//		const gs_texture_t* cubetex);
//
//	void (*gs_voltexture_destroy)(gs_texture_t* voltex);
//	uint32_t(*gs_voltexture_get_width)(const gs_texture_t* voltex);
//	uint32_t(*gs_voltexture_get_height)(const gs_texture_t* voltex);
//	uint32_t(*gs_voltexture_get_depth)(const gs_texture_t* voltex);
//	enum gs_color_format(*gs_voltexture_get_color_format)(
//		const gs_texture_t* voltex);
//
//	void (*gs_stagesurface_destroy)(gs_stagesurf_t* stagesurf);
//	uint32_t(*gs_stagesurface_get_width)(const gs_stagesurf_t* stagesurf);
//	uint32_t(*gs_stagesurface_get_height)(const gs_stagesurf_t* stagesurf);
//	enum gs_color_format(*gs_stagesurface_get_color_format)(
//		const gs_stagesurf_t* stagesurf);
//	bool (*gs_stagesurface_map)(gs_stagesurf_t* stagesurf, uint8_t** data,
//		uint32_t* linesize);
//	void (*gs_stagesurface_unmap)(gs_stagesurf_t* stagesurf);
//
//	void (*gs_zstencil_destroy)(gs_zstencil_t* zstencil);
//
//	void (*gs_samplerstate_destroy)(gs_samplerstate_t* samplerstate);
//
//	void (*gs_vertexbuffer_destroy)(gs_vertbuffer_t* vertbuffer);
//	void (*gs_vertexbuffer_flush)(gs_vertbuffer_t* vertbuffer);
//	void (*gs_vertexbuffer_flush_direct)(gs_vertbuffer_t* vertbuffer,
//		const struct gs_vb_data* data);
//	struct gs_vb_data* (*gs_vertexbuffer_get_data)(
//		const gs_vertbuffer_t* vertbuffer);
//
//	void (*gs_indexbuffer_destroy)(gs_indexbuffer_t* indexbuffer);
//	void (*gs_indexbuffer_flush)(gs_indexbuffer_t* indexbuffer);
//	void (*gs_indexbuffer_flush_direct)(gs_indexbuffer_t* indexbuffer,
//		const void* data);
//	void* (*gs_indexbuffer_get_data)(const gs_indexbuffer_t* indexbuffer);
//	size_t(*gs_indexbuffer_get_num_indices)(
//		const gs_indexbuffer_t* indexbuffer);
//	enum gs_index_type(*gs_indexbuffer_get_type)(
//		const gs_indexbuffer_t* indexbuffer);
//
//	void (*gs_timer_destroy)(gs_timer_t* timer);
//	void (*gs_timer_begin)(gs_timer_t* timer);
//	void (*gs_timer_end)(gs_timer_t* timer);
//	bool (*gs_timer_get_data)(gs_timer_t* timer, uint64_t* ticks);
//	void (*gs_timer_range_destroy)(gs_timer_range_t* range);
//	bool (*gs_timer_range_begin)(gs_timer_range_t* range);
//	bool (*gs_timer_range_end)(gs_timer_range_t* range);
//	bool (*gs_timer_range_get_data)(gs_timer_range_t* range, bool* disjoint,
//		uint64_t* frequency);
//
//	void (*gs_shader_destroy)(gs_shader_t* shader);
//	int (*gs_shader_get_num_params)(const gs_shader_t* shader);
//	gs_sparam_t* (*gs_shader_get_param_by_idx)(gs_shader_t* shader,
//		uint32_t param);
//	gs_sparam_t* (*gs_shader_get_param_by_name)(gs_shader_t* shader,
//		const char* name);
//	gs_sparam_t* (*gs_shader_get_viewproj_matrix)(const gs_shader_t* shader);
//	gs_sparam_t* (*gs_shader_get_world_matrix)(const gs_shader_t* shader);
//	void (*gs_shader_get_param_info)(const gs_sparam_t* param,
//		struct gs_shader_param_info* info);
//	void (*gs_shader_set_bool)(gs_sparam_t* param, bool val);
//	void (*gs_shader_set_float)(gs_sparam_t* param, float val);
//	void (*gs_shader_set_int)(gs_sparam_t* param, int val);
//	void (*gs_shader_set_matrix3)(gs_sparam_t* param,
//		const struct matrix3* val);
//	void (*gs_shader_set_matrix4)(gs_sparam_t* param,
//		const struct matrix4* val);
//	void (*gs_shader_set_vec2)(gs_sparam_t* param, const struct vec2* val);
//	void (*gs_shader_set_vec3)(gs_sparam_t* param, const struct vec3* val);
//	void (*gs_shader_set_vec4)(gs_sparam_t* param, const struct vec4* val);
//	void (*gs_shader_set_texture)(gs_sparam_t* param, gs_texture_t* val);
//	void (*gs_shader_set_val)(gs_sparam_t* param, const void* val,
//		size_t size);
//	void (*gs_shader_set_default)(gs_sparam_t* param);
//	void (*gs_shader_set_next_sampler)(gs_sparam_t* param,
//		gs_samplerstate_t* sampler);
//
//	bool (*device_nv12_available)(gs_device_t* device);
//	bool (*device_p010_available)(gs_device_t* device);
//
//	bool (*device_is_monitor_hdr)(gs_device_t* device, void* monitor);
//
//	void (*device_debug_marker_begin)(gs_device_t* device,
//		const char* markername,
//		const float color[4]);
//	void (*device_debug_marker_end)(gs_device_t* device);
//
#ifdef __APPLE__
//	/* OSX/Cocoa specific functions */
//	gs_texture_t* (*device_texture_create_from_iosurface)(gs_device_t* dev,
//		void* iosurf);
//	gs_texture_t* (*device_texture_open_shared)(gs_device_t* dev,
//		uint32_t handle);
//	bool (*gs_texture_rebind_iosurface)(gs_texture_t* texture,
//		void* iosurf);
//	bool (*device_shared_texture_available)(void);
//
#elif _WIN32
//	bool (*device_gdi_texture_available)(void);
//	bool (*device_shared_texture_available)(void);
//
//	bool (*device_get_duplicator_monitor_info)(
//		gs_device_t* device, int monitor_idx,
//		struct gs_monitor_info* monitor_info);
//	int (*device_duplicator_get_monitor_index)(gs_device_t* device,
//		void* monitor);
//
//	gs_duplicator_t* (*device_duplicator_create)(gs_device_t* device,
//		int monitor_idx);
//	void (*gs_duplicator_destroy)(gs_duplicator_t* duplicator);
//
//	bool (*gs_duplicator_update_frame)(gs_duplicator_t* duplicator);
//	gs_texture_t* (*gs_duplicator_get_texture)(gs_duplicator_t* duplicator);
//
//	uint32_t(*gs_get_adapter_count)(void);
//
//	gs_texture_t* (*device_texture_create_gdi)(gs_device_t* device,
//		uint32_t width,
//		uint32_t height);
//
//	void* (*gs_texture_get_dc)(gs_texture_t* gdi_tex);
//	void (*gs_texture_release_dc)(gs_texture_t* gdi_tex);
//
    virtual FGraphicSubsystemTexture* DeviceTextureOpenShared(FGraphicSubsystemDevice* device,uint32_t handle)=0;
//	gs_texture_t* (*device_texture_open_nt_shared)(gs_device_t* device,
//		uint32_t handle);
//	uint32_t(*device_texture_get_shared_handle)(gs_texture_t* tex);
//	gs_texture_t* (*device_texture_wrap_obj)(gs_device_t* device,
//		void* obj);
//	int (*device_texture_acquire_sync)(gs_texture_t* tex, uint64_t key,
//		uint32_t ms);
//	int (*device_texture_release_sync)(gs_texture_t* tex, uint64_t key);
//	bool (*device_texture_create_nv12)(gs_device_t* device,
//		gs_texture_t** tex_y,
//		gs_texture_t** tex_uv,
//		uint32_t width, uint32_t height,
//		uint32_t flags);
//	bool (*device_texture_create_p010)(gs_device_t* device,
//		gs_texture_t** tex_y,
//		gs_texture_t** tex_uv,
//		uint32_t width, uint32_t height,
//		uint32_t flags);
//
//	gs_stagesurf_t* (*device_stagesurface_create_nv12)(gs_device_t* device,
//		uint32_t width,
//		uint32_t height);
//	gs_stagesurf_t* (*device_stagesurface_create_p010)(gs_device_t* device,
//		uint32_t width,
//		uint32_t height);
//	void (*device_register_loss_callbacks)(
//		gs_device_t* device, const struct gs_device_loss* callbacks);
//	void (*device_unregister_loss_callbacks)(gs_device_t* device,
//		void* data);
#elif __linux__
//	struct gs_texture* (*device_texture_create_from_dmabuf)(
//		gs_device_t* device, unsigned int width, unsigned int height,
//		uint32_t drm_format, enum gs_color_format color_format,
//		uint32_t n_planes, const int* fds, const uint32_t* strides,
//		const uint32_t* offsets, const uint64_t* modifiers);
//	bool (*device_query_dmabuf_capabilities)(
//		gs_device_t* device, enum gs_dmabuf_flags* dmabuf_flags,
//		uint32_t** drm_formats, size_t* n_formats);
//	bool (*device_query_dmabuf_modifiers_for_format)(gs_device_t* device,
//		uint32_t drm_format,
//		uint64_t** modifiers,
//		size_t* n_modifiers);
//	struct gs_texture* (*device_texture_create_from_pixmap)(
//		gs_device_t* device, uint32_t width, uint32_t height,
//		enum gs_color_format color_format, uint32_t target,
//		void* pixmap);
#endif
};

GRAPHIC_SUBSYSTEM_EXPORT FGraphicSubsystem* GetGraphicSubsystem(EGraphicSubsystem);

static inline uint32_t GetColorFormatBitPerPixel(enum EGraphicSubsystemColorFormat format)
{
	switch (format) {
	case EGraphicSubsystemColorFormat::DXT1:
		return 4;
	case EGraphicSubsystemColorFormat::A8:
	case EGraphicSubsystemColorFormat::R8:
	case EGraphicSubsystemColorFormat::DXT3:
	case EGraphicSubsystemColorFormat::DXT5:
		return 8;
	case EGraphicSubsystemColorFormat::R16:
	case EGraphicSubsystemColorFormat::R16F:
	case EGraphicSubsystemColorFormat::R8G8:
		return 16;
	case EGraphicSubsystemColorFormat::RGBA:
	case EGraphicSubsystemColorFormat::BGRX:
	case EGraphicSubsystemColorFormat::BGRA:
	case EGraphicSubsystemColorFormat::R10G10B10A2:
	case EGraphicSubsystemColorFormat::RG16F:
	case EGraphicSubsystemColorFormat::R32F:
	case EGraphicSubsystemColorFormat::RGBA_UNORM:
	case EGraphicSubsystemColorFormat::BGRX_UNORM:
	case EGraphicSubsystemColorFormat::BGRA_UNORM:
	case EGraphicSubsystemColorFormat::RG16:
		return 32;
	case EGraphicSubsystemColorFormat::RGBA16:
	case EGraphicSubsystemColorFormat::RGBA16F:
	case EGraphicSubsystemColorFormat::RG32F:
		return 64;
	case EGraphicSubsystemColorFormat::RGBA32F:
		return 128;
	case EGraphicSubsystemColorFormat::UNKNOWN:
		return 0;
	}

	return 0;
}

static inline uint32_t GetTotalLevels(uint32_t width, uint32_t height,uint32_t depth)
{
	uint32_t size = width > height ? width : height;
	size = size > depth ? size : depth;
	uint32_t num_levels = 1;
	while (size > 1) {
		size /= 2;
		num_levels++;
	}
	return num_levels;
}