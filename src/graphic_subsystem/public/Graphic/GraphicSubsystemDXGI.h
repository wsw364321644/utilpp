#pragma once
#include <wrl.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include "Graphic/GraphicSubsystem.h"
using namespace Microsoft::WRL;
class FGraphicSubsystemDXGIDevice :public  FGraphicSubsystemDevice {
public:
    FGraphicSubsystemDXGIDevice(uint32_t adapterIdx);
    ComPtr<IDXGIFactory1> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    uint32_t AdpIdx{ 0 };
};

class FGraphicSubsystemDXGITexture :public FGraphicSubsystemTexture {
public:
	FGraphicSubsystemDXGITexture(FGraphicSubsystemDevice* device,EGraphicSubsystemTextureType type) :FGraphicSubsystemTexture(device),TextureType(type) {}
	virtual bool IsNTShared()const = 0;
	uint32_t GetByteSize() const override;
	virtual uint64_t GetSharedHandle() const {
		return (uint64_t)SharedHandle;
	};
	DXGI_FORMAT DXGIFormatResource{ DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT DXGIFormatView{ DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT DXGIFormatViewLinear{ DXGI_FORMAT_UNKNOWN };
	uint32_t Levels{0};
	HANDLE SharedHandle{NULL};
	EGraphicSubsystemTextureType TextureType{ EGraphicSubsystemTextureType ::TEXTURE_1D};
};

class FGraphicSubsystemDXGI :public FGraphicSubsystem {
    virtual EGraphicSubsystemError DeviceEnumAdapters(bool (*callback)(void* param, const char* name, uint32_t id), void* param) override;
};

void LogD3DAdapters();
static inline EGraphicSubsystemColorFormat ConvertDXGITextureFormat(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_A8_UNORM:
		return EGraphicSubsystemColorFormat::A8;
	case DXGI_FORMAT_R8_UNORM:
		return EGraphicSubsystemColorFormat::R8;
	case DXGI_FORMAT_R8G8_UNORM:
		return EGraphicSubsystemColorFormat::R8G8;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return EGraphicSubsystemColorFormat::RGBA;
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return EGraphicSubsystemColorFormat::BGRX;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return EGraphicSubsystemColorFormat::BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return EGraphicSubsystemColorFormat::R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return EGraphicSubsystemColorFormat::RGBA16;
	case DXGI_FORMAT_R16_UNORM:
		return EGraphicSubsystemColorFormat::R16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return EGraphicSubsystemColorFormat::RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return EGraphicSubsystemColorFormat::RGBA32F;
	case DXGI_FORMAT_R16G16_FLOAT:
		return EGraphicSubsystemColorFormat::RG16F;
	case DXGI_FORMAT_R32G32_FLOAT:
		return EGraphicSubsystemColorFormat::RG32F;
	case DXGI_FORMAT_R16_FLOAT:
		return EGraphicSubsystemColorFormat::R16F;
	case DXGI_FORMAT_R32_FLOAT:
		return EGraphicSubsystemColorFormat::R32F;
	case DXGI_FORMAT_BC1_UNORM:
		return EGraphicSubsystemColorFormat::DXT1;
	case DXGI_FORMAT_BC2_UNORM:
		return EGraphicSubsystemColorFormat::DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return EGraphicSubsystemColorFormat::DXT5;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return EGraphicSubsystemColorFormat::RGBA_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return EGraphicSubsystemColorFormat::BGRX_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return EGraphicSubsystemColorFormat::BGRA_UNORM;
	case DXGI_FORMAT_R16G16_UNORM:
		return EGraphicSubsystemColorFormat::RG16;
	}

	return EGraphicSubsystemColorFormat::UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSTextureFormatResource(EGraphicSubsystemColorFormat format)
{
	switch (format) {
	case EGraphicSubsystemColorFormat::UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
	case EGraphicSubsystemColorFormat::A8:
		return DXGI_FORMAT_A8_UNORM;
	case EGraphicSubsystemColorFormat::R8:
		return DXGI_FORMAT_R8_UNORM;
	case EGraphicSubsystemColorFormat::RGBA:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case EGraphicSubsystemColorFormat::BGRX:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case EGraphicSubsystemColorFormat::BGRA:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case EGraphicSubsystemColorFormat::R10G10B10A2:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case EGraphicSubsystemColorFormat::RGBA16:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case EGraphicSubsystemColorFormat::R16:
		return DXGI_FORMAT_R16_UNORM;
	case EGraphicSubsystemColorFormat::RGBA16F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case EGraphicSubsystemColorFormat::RGBA32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case EGraphicSubsystemColorFormat::RG16F:
		return DXGI_FORMAT_R16G16_FLOAT;
	case EGraphicSubsystemColorFormat::RG32F:
		return DXGI_FORMAT_R32G32_FLOAT;
	case EGraphicSubsystemColorFormat::R16F:
		return DXGI_FORMAT_R16_FLOAT;
	case EGraphicSubsystemColorFormat::R32F:
		return DXGI_FORMAT_R32_FLOAT;
	case EGraphicSubsystemColorFormat::DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case EGraphicSubsystemColorFormat::DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case EGraphicSubsystemColorFormat::DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case EGraphicSubsystemColorFormat::R8G8:
		return DXGI_FORMAT_R8G8_UNORM;
	case EGraphicSubsystemColorFormat::RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case EGraphicSubsystemColorFormat::BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case EGraphicSubsystemColorFormat::BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case EGraphicSubsystemColorFormat::RG16:
		return DXGI_FORMAT_R16G16_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}


static inline DXGI_FORMAT ConvertGSTextureFormatView(EGraphicSubsystemColorFormat format)
{
	switch (format) {
	case EGraphicSubsystemColorFormat::RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case EGraphicSubsystemColorFormat::BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case EGraphicSubsystemColorFormat::BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}


static inline DXGI_FORMAT ConvertGSTextureFormatViewLinear(EGraphicSubsystemColorFormat format)
{
	switch (format) {
	case EGraphicSubsystemColorFormat::RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case EGraphicSubsystemColorFormat::BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	case EGraphicSubsystemColorFormat::BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}