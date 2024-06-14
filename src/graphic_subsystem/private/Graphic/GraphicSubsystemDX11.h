#pragma once
#include "Graphic/GraphicSubsystemDXGI.h"



class FGraphicSubsystemDX11 :public FGraphicSubsystemDXGI {
	virtual const char* DeviceGetName(void) override;
	virtual EGraphicSubsystem DeviceGetType(void) override;

	virtual EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter) override;
	virtual void DeviceDestroy(FGraphicSubsystemDevice* device) override;

	virtual FGraphicSubsystemTexture* DeviceTextureOpenShared(FGraphicSubsystemDevice* device, uint32_t handle) override;
};