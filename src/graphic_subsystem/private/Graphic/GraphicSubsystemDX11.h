#pragma once
#include "Graphic/GraphicSubsystemDXGI.h"



class FGraphicSubsystemDX11 :public FGraphicSubsystemDXGI {
	const char* DeviceGetName(void) override;
	EGraphicSubsystem DeviceGetType(void) override;

	EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter) override;
	void DeviceDestroy(FGraphicSubsystemDevice* device) override;

	FGraphicSubsystemTexture* DeviceTextureOpenShared(FGraphicSubsystemDevice* device, uint32_t handle) override;
};