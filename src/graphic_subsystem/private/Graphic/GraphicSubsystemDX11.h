#pragma once
#include "Graphic/GraphicSubsystemDXGI.h"


class FGraphicSubsystemDX11 :public FGraphicSubsystemDXGI {
	const char* DeviceGetName(void) override;
	EGraphicSubsystem DeviceGetType(void) override;

	EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter) override;
	void DeviceDestroy(FGraphicSubsystemDevice* device) override;

	FGraphicSubsystemTexture* DeviceTextureCreate(FGraphicSubsystemDevice* device, uint32_t width, uint32_t height,
		enum EGraphicSubsystemColorFormat color_format, uint32_t levels,const uint8_t** data, TextureFlag_t flags)override;
	void DeviceCopyTextureRegion(FGraphicSubsystemDevice* device, FGraphicSubsystemTexture* dst, uint32_t dst_x, uint32_t dst_y,
		FGraphicSubsystemTexture* src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)override;
	bool TextureMap(FGraphicSubsystemTexture* tex, uint8_t** ptr, uint32_t* linesize) override;
	bool TextureUnmap(FGraphicSubsystemTexture* tex) override;

	FGraphicSubsystemTexture* DeviceTextureOpenShared(FGraphicSubsystemDevice* device, uint32_t handle) override;
};