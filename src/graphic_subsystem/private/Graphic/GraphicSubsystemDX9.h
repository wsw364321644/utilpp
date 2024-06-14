#pragma once
#include "Graphic/GraphicSubsystem.h"



class FGraphicSubsystemDX9 :public FGraphicSubsystem {
public:
	virtual const char* DeviceGetName(void) override;
	virtual EGraphicSubsystem DeviceGetType(void) override;
	virtual EGraphicSubsystemError DeviceEnumAdapters(bool (*callback)(void* param, const char* name, uint32_t id), void* param) override;
	virtual EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter) override;
};