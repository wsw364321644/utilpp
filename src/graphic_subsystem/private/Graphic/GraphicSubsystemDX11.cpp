#include "GraphicSubsystemDX11.h"
#include <string_convert.h>
#include <comdef.h>	
#include <dxgi.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <LoggerHelper.h>
#include <wrl.h>
using namespace Microsoft::WRL;
const static D3D_FEATURE_LEVEL featureLevels[] = {
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
};

static bool increase_maximum_frame_latency(ComPtr<ID3D11Device> device)
{
	ComPtr<IDXGIDevice1> dxgiDevice;
	auto hr = device.As(&dxgiDevice);
	if (FAILED(hr)) {
		SIMPLELOG_LOGGER_DEBUG(nullptr, " Failed to get IDXGIDevice1");
		return false;
	}
	hr = dxgiDevice->SetMaximumFrameLatency(16);
	if (FAILED(hr)) {
		SIMPLELOG_LOGGER_DEBUG(nullptr, "SetMaximumFrameLatency failed");
		return false;
	}
	SIMPLELOG_LOGGER_DEBUG(nullptr, "DXGI increase maximum frame latency success");
	return true;
}


static bool CheckFormat(ID3D11Device* device, DXGI_FORMAT format)
{
	constexpr UINT required = D3D11_FORMAT_SUPPORT_TEXTURE2D |
		D3D11_FORMAT_SUPPORT_RENDER_TARGET;

	UINT support = 0;
	return SUCCEEDED(device->CheckFormatSupport(format, &support)) &&
		((support & required) == required);
}

class FGraphicSubsystemDX11Device :public FGraphicSubsystemDXGIDevice {
public:
	FGraphicSubsystemDX11Device(uint32_t adapterIdx);
	virtual ~FGraphicSubsystemDX11Device();
	ComPtr<ID3D11Device> Device;
	ComPtr<ID3D11DeviceContext> Context;
private:
	bool nv12Supported;
	bool p010Supported;
	void InitDevice();
	bool HasBadNV12Output();
};

FGraphicSubsystemDX11Device::FGraphicSubsystemDX11Device(uint32_t adapterIdx) :FGraphicSubsystemDXGIDevice(adapterIdx)
{
	InitDevice();
}

FGraphicSubsystemDX11Device::~FGraphicSubsystemDX11Device()
{
	Context->ClearState();
}

void FGraphicSubsystemDX11Device::InitDevice()
{
	std::wstring adapterName;
	DXGI_ADAPTER_DESC desc;
	D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_10_0;
	HRESULT hr = 0;

	uint32_t createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	adapterName = (Adapter->GetDesc(&desc) == S_OK) ? desc.Description: L"<unknown>";
	auto adapterNameu8=U16ToU8((const char16_t*)adapterName.c_str());
	SIMPLELOG_LOGGER_INFO(nullptr, "Loading up D3D11 on adapter {} ({})", adapterNameu8, AdpIdx);


	hr = D3D11CreateDevice(Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL,
		createFlags, featureLevels,
		sizeof(featureLevels) /
		sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION, &Device, &levelUsed,
		&Context);
	if (FAILED(hr))
		throw _com_error( hr);

	SIMPLELOG_LOGGER_INFO(nullptr, "D3D11 loaded successfully, feature level used: {}", (unsigned int)levelUsed);

	/* prevent stalls sometimes seen in Present calls */
	if (!increase_maximum_frame_latency(Device)) {
		SIMPLELOG_LOGGER_INFO(nullptr, "DXGI increase maximum frame latency failed");
	}

	/* ---------------------------------------- */
	/* check for nv12 texture output support    */

	nv12Supported = false;
	p010Supported = false;

	/* WARP NV12 support is suspected to be buggy on older Windows */
	if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c) {
		return;
	}

	ComPtr<ID3D11Device1> d3d11_1;
	hr=Device.As(&d3d11_1);
	if (FAILED(hr))
		return;

	/* needs to support extended resource sharing */
	D3D11_FEATURE_DATA_D3D11_OPTIONS opts = {};
	hr = d3d11_1->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &opts,
		sizeof(opts));
	if (FAILED(hr) || !opts.ExtendedResourceSharing) {
		return;
	}

	nv12Supported = CheckFormat(Device.Get(), DXGI_FORMAT_NV12) &&
		!HasBadNV12Output();
	p010Supported = nv12Supported && CheckFormat(Device.Get(), DXGI_FORMAT_P010);
}

bool FGraphicSubsystemDX11Device::HasBadNV12Output()
{
	//todo
	return false;
}





class FGraphicSubsystemDX11Texture2D :public FGraphicSubsystemDXGITexture {
public:
	FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t handle, bool ntHandle=false);

	uint32_t GetWidth() override { return Texture2DDesc.Width; }
	uint32_t GetHeight() override { return Texture2DDesc.Height; }
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	ComPtr<ID3D11Texture2D> Texture;
	ComPtr<ID3D11ShaderResourceView> ShaderRes;
	ComPtr<ID3D11ShaderResourceView> ShaderResLinear;
	bool bShared{ false };
	bool bGenMipmaps{false};
	uint32_t SharedHandle;
	D3D11_SHADER_RESOURCE_VIEW_DESC ViewDesc{};
	D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescLinear{};
private:
	
	void InitResourceView(FGraphicSubsystemDX11Device* device) noexcept(false);
};

FGraphicSubsystemDX11Texture2D::FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t handle, bool ntHandle)
	:FGraphicSubsystemDXGITexture(EGraphicSubsystemTextureType::TEXTURE_2D),bShared(true), SharedHandle(handle)
{
	HRESULT hr;
	if (ntHandle) {
		ComPtr < ID3D11Device1> dev;
		hr = device->Device.As(&dev);

		if (FAILED(hr))
			throw _com_error(hr);

		hr = dev->OpenSharedResource1((HANDLE)(uintptr_t)handle, IID_PPV_ARGS(&Texture));
	}
	else {
		hr = device->Device->OpenSharedResource((HANDLE)(uintptr_t)handle, IID_PPV_ARGS(&Texture));
	}

	if (FAILED(hr))
		throw _com_error(hr);

	Texture->GetDesc(&Texture2DDesc);

	ColorFormat = ConvertDXGITextureFormat(Texture2DDesc.Format);
	Levels = 1;

	DXGIFormatResource = ConvertGSTextureFormatResource(ColorFormat);
	DXGIFormatView = ConvertGSTextureFormatView(ColorFormat);
	DXGIFormatViewLinear = ConvertGSTextureFormatViewLinear(ColorFormat);

	InitResourceView(device);
}

void FGraphicSubsystemDX11Texture2D::InitResourceView(FGraphicSubsystemDX11Device* device) noexcept(false)
{
	HRESULT hr;
	memset(&ViewDesc, 0, sizeof(ViewDesc));
	ViewDesc.Format = DXGIFormatView;

	if (TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY) {
		ViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		ViewDesc.TextureCube.MipLevels = bGenMipmaps || !Levels ? -1
			: Levels;
	}
	else {
		ViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		ViewDesc.Texture2D.MipLevels = bGenMipmaps || !Levels ? -1
			: Levels;
	}

	hr = device->Device->CreateShaderResourceView(Texture.Get(), &ViewDesc,
		&ShaderRes);
	if (FAILED(hr))
		throw _com_error( hr);

	ViewDescLinear = ViewDesc;
	ViewDescLinear.Format = DXGIFormatViewLinear;

	if (DXGIFormatView == DXGIFormatViewLinear) {
		ShaderResLinear = ShaderRes;
	}
	else {
		hr = device->Device->CreateShaderResourceView(
			Texture.Get(), &ViewDescLinear, &ShaderResLinear);
		if (FAILED(hr))
			throw _com_error(hr);
	}
}

const char* FGraphicSubsystemDX11::DeviceGetName(void)
{
	return "Direct3D 11";
}

EGraphicSubsystem FGraphicSubsystemDX11::DeviceGetType(void)
{
	return EGraphicSubsystem::DX11;
}

EGraphicSubsystemError FGraphicSubsystemDX11::DeviceCreate(FGraphicSubsystemDevice** p_device, uint32_t adapter)
{
	FGraphicSubsystemDX11Device* device = NULL;
	EGraphicSubsystemError errorcode = EGraphicSubsystemError::OK;

	try {
		LogD3DAdapters();
		device = new FGraphicSubsystemDX11Device(adapter);
	}
	catch (const _com_error& error) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceCreate (D3D11): {} ({})", error.ErrorMessage(), error.Error());
		errorcode = EGraphicSubsystemError::DXGIFailed;
	}
	catch (...) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceCreate (D3D11)) UNKNOWN");
		errorcode = EGraphicSubsystemError::UNKNOWN;
	}

	*p_device = device;
	return EGraphicSubsystemError::OK;
}

void FGraphicSubsystemDX11::DeviceDestroy(FGraphicSubsystemDevice* device)
{
	delete device;
}

FGraphicSubsystemTexture* FGraphicSubsystemDX11::DeviceTextureOpenShared(FGraphicSubsystemDevice* device, uint32_t handle)
{
	FGraphicSubsystemDX11Texture2D* texture = nullptr;
	try {
		texture = new FGraphicSubsystemDX11Texture2D(dynamic_cast<FGraphicSubsystemDX11Device*>(device), handle);
	}
	catch (const _com_error& error) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceTextureOpenShared (D3D11): {} ({})", error.ErrorMessage(), error.Error());
	}
	catch (...) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceTextureOpenShared (D3D11)) UNKNOWN");
	}

	return texture;
}


