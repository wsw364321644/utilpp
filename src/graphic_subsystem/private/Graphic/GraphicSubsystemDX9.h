#pragma once
#include "Graphic/GraphicSubsystem.h"



class FGraphicSubsystemDX9 :public FGraphicSubsystem {
public:
	const char* DeviceGetName(void) override;
	EGraphicSubsystem DeviceGetType(void) override;
	EGraphicSubsystemError DeviceEnumAdapters(bool (*callback)(void* param, const char* name, uint32_t id), void* param) override;
	EGraphicSubsystemError DeviceCreate(FGraphicSubsystemDevice** device, uint32_t adapter) override;
};