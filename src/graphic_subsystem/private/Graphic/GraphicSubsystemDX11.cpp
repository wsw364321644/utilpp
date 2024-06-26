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


class FGraphicSubsystemDX11Texture2D;
class FGraphicSubsystemDX11Device;

class FGraphicSubsystemDX11Texture2D :public FGraphicSubsystemDXGITexture {
public:
	FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t handle, bool ntHandle = false);
	FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t width, uint32_t height,
		EGraphicSubsystemColorFormat color_format, uint32_t levels, const uint8_t** data, TextureFlag_t flags, EGraphicSubsystemTextureType type);
	FGraphicSubsystemDX11Device* GetDX11Device();
	uint32_t GetWidth()const  override { return Texture2DDesc.Width; }
	uint32_t GetHeight()const override { return Texture2DDesc.Height; }
	EGraphicSubsystemTextureType GetType() const override { return TextureType; }
	EGraphicSubsystemColorFormat GetColorFormat() const override { return ConvertDXGITextureFormat(Texture2DDesc.Format); }

	bool IsNTShared()const override {
		return Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
	}
	bool IsShared()const override {
		return IsNTShared() ||
			Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED;
	}
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	ComPtr<ID3D11Texture2D> Texture;
	ComPtr<ID3D11ShaderResourceView> ShaderRes;
	ComPtr<ID3D11ShaderResourceView> ShaderResLinear;
	ComPtr<IDXGISurface1> GDISurface;
	ComPtr<ID3D11RenderTargetView> RenderTarget[6];
	ComPtr<ID3D11RenderTargetView> RenderTargetLinear[6];
	D3D11_SHADER_RESOURCE_VIEW_DESC ViewDesc{};
	D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescLinear{};

	std::vector<std::vector<uint8_t>> Data;
	std::vector<D3D11_SUBRESOURCE_DATA> SRD;
private:

	bool AcquireSync(uint64_t Key, DWORD dwMilliseconds = INFINITE)noexcept(false);
	bool ReleaseSync(uint64_t Key)noexcept(false);
	void InitResourceView() noexcept(false);
	void InitTexture(EGraphicSubsystemColorFormat colorFormat, TextureFlag_t& Flags, const uint8_t* const* data) noexcept(false);
	void InitRenderTargets()noexcept(false);
	void BackupTexture(const uint8_t* const* data);
	void InitSRD(std::vector<D3D11_SUBRESOURCE_DATA>& srd);
};



class FGraphicSubsystemDX11Device :public FGraphicSubsystemDXGIDevice {
public:
	FGraphicSubsystemDX11Device(uint32_t adapterIdx);
	~FGraphicSubsystemDX11Device() override;

	void CopyTex(FGraphicSubsystemDX11Texture2D* dst, uint32_t dst_x,
		uint32_t dst_y, FGraphicSubsystemDX11Texture2D* src, uint32_t src_x,
		uint32_t src_y, uint32_t src_w, uint32_t src_h);
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

void FGraphicSubsystemDX11Device::CopyTex(FGraphicSubsystemDX11Texture2D* dst, uint32_t dst_x, uint32_t dst_y, FGraphicSubsystemDX11Texture2D* src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 &&
		src_w == 0 && src_h == 0) {
		Context->CopyResource(dst->Texture.Get(), src->Texture.Get());
	}
	else {
		D3D11_BOX sbox;

		sbox.left = src_x;
		if (src_w > 0)
			sbox.right = src_x + src_w;
		else
			sbox.right = src->GetWidth() - 1;

		sbox.top = src_y;
		if (src_h > 0)
			sbox.bottom = src_y + src_h;
		else
			sbox.bottom = src->GetHeight() - 1;

		sbox.front = 0;
		sbox.back = 1;

		Context->CopySubresourceRegion(dst->Texture.Get(), 0, dst_x, dst_y, 0,
			src->Texture.Get(), 0, &sbox);
	}
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
		sizeof(featureLevels) /sizeof(D3D_FEATURE_LEVEL),
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





FGraphicSubsystemDX11Texture2D::FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t handle, bool ntHandle)
	:FGraphicSubsystemDXGITexture(device,EGraphicSubsystemTextureType::TEXTURE_2D)
{
	SharedHandle = (HANDLE)handle;
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

	
	Levels = 1;

	DXGIFormatResource = ConvertGSTextureFormatResource(GetColorFormat());
	DXGIFormatView = ConvertGSTextureFormatView(GetColorFormat());
	DXGIFormatViewLinear = ConvertGSTextureFormatViewLinear(GetColorFormat());

	InitResourceView();
}

FGraphicSubsystemDX11Texture2D::FGraphicSubsystemDX11Texture2D(FGraphicSubsystemDX11Device* device, uint32_t width, uint32_t height, 
	EGraphicSubsystemColorFormat color_format, uint32_t levels, const uint8_t** data, TextureFlag_t flags, EGraphicSubsystemTextureType type)
	:FGraphicSubsystemDXGITexture(device,type)
{
	Texture2DDesc.Width = width;
	Texture2DDesc.Height = height;
	DXGIFormatResource = ConvertGSTextureFormatResource(color_format);
	DXGIFormatView = ConvertGSTextureFormatView(color_format);
	DXGIFormatViewLinear = ConvertGSTextureFormatViewLinear(color_format);
	Levels = levels;
	InitTexture(color_format,flags,data);
	InitResourceView();
	if (flags.test(ETextureFlag::BIND_RENDER_TARGET)) {
		InitRenderTargets();
	}
}

FGraphicSubsystemDX11Device* FGraphicSubsystemDX11Texture2D::GetDX11Device()
{
	return dynamic_cast<FGraphicSubsystemDX11Device*>(Device);
}

bool FGraphicSubsystemDX11Texture2D::AcquireSync(uint64_t Key, DWORD dwMilliseconds)
{
	if (!(Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)) {
		return true;
	}
	if (bAcquired) {
		return true;
	}
	ComPtr<IDXGIKeyedMutex> km;
	auto hr = Texture.As(&km);
	if (FAILED(hr)) {
		throw _com_error(hr);
	}
	DWORD result = km->AcquireSync(Key, dwMilliseconds);
	if (result == WAIT_OBJECT_0)
		bAcquired = true;
	return bAcquired;
}

bool FGraphicSubsystemDX11Texture2D::ReleaseSync(uint64_t Key) noexcept(false)
{
	if (!(Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)) {
		return true;
	}
	if (!bAcquired) {
		return false;
	}
	ComPtr<IDXGIKeyedMutex> km;
	auto hr = Texture.As(&km);
	if (FAILED(hr)) {
		throw _com_error(hr);
	}
	DWORD result = km->ReleaseSync(Key);
	if (result == WAIT_OBJECT_0)
		bAcquired = false;
	return bAcquired == false;
}

void FGraphicSubsystemDX11Texture2D::InitResourceView() noexcept(false)
{
	HRESULT hr;
	memset(&ViewDesc, 0, sizeof(ViewDesc));
	ViewDesc.Format = DXGIFormatView;

	if (TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY) {
		ViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		ViewDesc.TextureCube.MipLevels = (Texture2DDesc.MiscFlags& D3D11_RESOURCE_MISC_GENERATE_MIPS) || !Levels ? 0: Levels;
	}
	else {
		ViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		ViewDesc.Texture2D.MipLevels = (Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) || !Levels ? 0: Levels;
	}
	
	hr = GetDX11Device()->Device->CreateShaderResourceView(Texture.Get(), &ViewDesc,
		&ShaderRes);
	if (FAILED(hr))
		throw _com_error( hr);

	ViewDescLinear = ViewDesc;
	ViewDescLinear.Format = DXGIFormatViewLinear;

	if (DXGIFormatView == DXGIFormatViewLinear) {
		ShaderResLinear = ShaderRes;
	}
	else {
		hr = GetDX11Device()->Device->CreateShaderResourceView(
			Texture.Get(), &ViewDescLinear, &ShaderResLinear);
		if (FAILED(hr))
			throw _com_error(hr);
	}
}

void FGraphicSubsystemDX11Texture2D::InitTexture(EGraphicSubsystemColorFormat colorFormat,TextureFlag_t& Flags,const uint8_t* const* data) noexcept(false)
{
	auto Device = dynamic_cast<FGraphicSubsystemDX11Device*>(this->Device);
	HRESULT hr;

	Texture2DDesc.MipLevels = Flags.test(ETextureFlag::MISC_GENERATE_MIPS) ? 0 : Levels;
	Texture2DDesc.ArraySize = TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY ? 6 : 1;
	Texture2DDesc.Format = Flags.test(ETextureFlag::TWO_PLANE) ? ((colorFormat == EGraphicSubsystemColorFormat::R16) ? DXGI_FORMAT_P010
		: DXGI_FORMAT_NV12)
		: DXGIFormatResource;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.CPUAccessFlags =(Flags.test(ETextureFlag::CPU_ACCESS_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0)|
		(Flags.test(ETextureFlag::CPU_ACCESS_READ) ? D3D11_CPU_ACCESS_READ : 0);
	Texture2DDesc.Usage = Flags.test(ETextureFlag::CPU_ACCESS_WRITE)|| Flags.test(ETextureFlag::CPU_ACCESS_READ) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

	if (TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY)
		Texture2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (Flags.test(ETextureFlag::BIND_RENDER_TARGET) || Flags.test(ETextureFlag::MISC_GDI_COMPATIBLE))
		Texture2DDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	if (Flags.test(ETextureFlag::MISC_GDI_COMPATIBLE))
		Texture2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	if (Flags.test(ETextureFlag::MISC_SHARED_NTHANDLE))
		Texture2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
	else if (Flags.test(ETextureFlag::MISC_SHARED))
		Texture2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;

	if (Flags.test(ETextureFlag::MISC_SHARED_KEYEDMUTEX))
		Texture2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	if (data) {
		BackupTexture(data);
		InitSRD(SRD);
	}

	hr = Device->Device->CreateTexture2D(&Texture2DDesc, data ? SRD.data() : NULL,
		&Texture);
	if (FAILED(hr))
		throw _com_error( hr);

	if (Flags.test(ETextureFlag::MISC_GDI_COMPATIBLE)) {
		hr = Texture.As(&GDISurface);
		if (FAILED(hr))
			throw _com_error(hr);
	}

	if (IsNTShared()) {
		ComPtr<IDXGIResource1> dxgi_res;
		Texture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
		hr = Texture.As(&dxgi_res);
		if (FAILED(hr)) {
			SIMPLELOG_LOGGER_WARN(nullptr, "InitTexture: Failed to query interface: {}", hr);
		}
		else {
			hr=dxgi_res->CreateSharedHandle(NULL,
				DXGI_SHARED_RESOURCE_READ,
				NULL,
				&SharedHandle);
			if (FAILED(hr)) {
				SIMPLELOG_LOGGER_WARN(nullptr, "CreateSharedHandle failed: {}", hr);
			}
		}
	}
	else if (IsShared()) {
		ComPtr<IDXGIResource> dxgi_res;
		Texture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
		hr = Texture.As(&dxgi_res);
		if (FAILED(hr)) {
			SIMPLELOG_LOGGER_WARN(nullptr, "InitTexture: Failed to query interface: {}", hr);
		}
		else {
			dxgi_res->GetSharedHandle(&SharedHandle);
			if (FAILED(hr)) {
				SIMPLELOG_LOGGER_WARN(nullptr, "GetSharedHandle failed: {}", hr);
			}
		}
	}
}

void FGraphicSubsystemDX11Texture2D::InitRenderTargets() noexcept(false)
{
	HRESULT hr;
	if (TextureType == EGraphicSubsystemTextureType::TEXTURE_2D) {
		D3D11_RENDER_TARGET_VIEW_DESC rtv;
		rtv.Format = DXGIFormatView;
		rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtv.Texture2D.MipSlice = 0;

		hr = GetDX11Device()->Device->CreateRenderTargetView(
			Texture.Get(), &rtv, &RenderTarget[0]);
		if (FAILED(hr))
			throw _com_error(hr);
		if (DXGIFormatView == DXGIFormatViewLinear) {
			RenderTargetLinear[0] = RenderTarget[0];
		}
		else {
			rtv.Format = DXGIFormatViewLinear;
			hr = GetDX11Device()->Device->CreateRenderTargetView(
				Texture.Get(), &rtv, &RenderTargetLinear[0]);
			if (FAILED(hr))
				throw _com_error(hr);
		}
	}
	else {
		D3D11_RENDER_TARGET_VIEW_DESC rtv;
		rtv.Format = DXGIFormatView;
		rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtv.Texture2DArray.MipSlice = 0;
		rtv.Texture2DArray.ArraySize = 1;

		for (UINT i = 0; i < 6; i++) {
			rtv.Texture2DArray.FirstArraySlice = i;
			hr = GetDX11Device()->Device->CreateRenderTargetView(
				Texture.Get(), &rtv, &RenderTarget[i]);
			if (FAILED(hr))
				throw _com_error( hr);
			if (DXGIFormatView == DXGIFormatViewLinear) {
				RenderTargetLinear[i] = RenderTarget[i];
			}
			else {
				rtv.Format = DXGIFormatViewLinear;
				hr = GetDX11Device()->Device->CreateRenderTargetView(
					Texture.Get(), &rtv,
					&RenderTargetLinear[i]);
				if (FAILED(hr))
					throw _com_error(hr);
			}
		}
	}
}

void FGraphicSubsystemDX11Texture2D::BackupTexture(const uint8_t* const* data)
{
	uint32_t textures = TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY ? 6 : 1;

	Data.resize(Levels * textures);

	for (uint32_t t = 0; t < textures; t++) {
		uint32_t w = Texture2DDesc.Width;
		uint32_t h = Texture2DDesc.Height;

		for (uint32_t lv = 0; lv < Levels; lv++) {
			uint32_t i = Levels * t + lv;
			if (!data[i])
				break;

			uint32_t texSize = GetByteSize();

			std::vector<uint8_t>& subData = Data[i];
			subData.resize(texSize);
			memcpy(&subData[0], data[i], texSize);

			if (w > 1)
				w /= 2;
			if (h > 1)
				h /= 2;
		}
	}
}

void FGraphicSubsystemDX11Texture2D::InitSRD(std::vector<D3D11_SUBRESOURCE_DATA>& srd)
{
	uint32_t rowSizeBytes = Texture2DDesc.Width * GetColorFormatBitPerPixel(GetColorFormat());
	uint32_t texSizeBytes = Texture2DDesc.Height * rowSizeBytes / 8;
	size_t textures = TextureType == EGraphicSubsystemTextureType::TEXTURE_2D_ARRAY ? 6:1;
	uint32_t actual_levels = Levels;
	size_t curTex = 0;

	if (!actual_levels)
		actual_levels = GetTotalLevels(Texture2DDesc.Width, Texture2DDesc.Height, 1);

	rowSizeBytes /= 8;

	for (size_t i = 0; i < textures; i++) {
		uint32_t newRowSize = rowSizeBytes;
		uint32_t newTexSize = texSizeBytes;

		for (uint32_t j = 0; j < actual_levels; j++) {
			D3D11_SUBRESOURCE_DATA newSRD;
			newSRD.pSysMem = Data[curTex++].data();
			newSRD.SysMemPitch = newRowSize;
			newSRD.SysMemSlicePitch = newTexSize;
			srd.push_back(newSRD);
			newRowSize /= 2;
			newTexSize /= 4;
		}
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

FGraphicSubsystemTexture* FGraphicSubsystemDX11::DeviceTextureCreate(FGraphicSubsystemDevice* _device, uint32_t width, uint32_t height, EGraphicSubsystemColorFormat color_format, uint32_t levels, const uint8_t** data, TextureFlag_t flags)
{
	FGraphicSubsystemDX11Device* device = dynamic_cast<FGraphicSubsystemDX11Device*>(_device);
	FGraphicSubsystemTexture* texture{nullptr};
	try {
		texture = new FGraphicSubsystemDX11Texture2D(device, width, height, color_format,
			levels, data, flags, EGraphicSubsystemTextureType::TEXTURE_2D);
	}
	catch (const _com_error& error) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceCreate (D3D11): {} ({})", error.ErrorMessage(), error.Error());
	}
	catch (...) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "DeviceCreate (D3D11)) UNKNOWN");
	}

	return texture;
}

void FGraphicSubsystemDX11::DeviceCopyTextureRegion(FGraphicSubsystemDevice* device, FGraphicSubsystemTexture* dst, uint32_t dst_x, uint32_t dst_y, FGraphicSubsystemTexture* src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	try {
		FGraphicSubsystemDX11Texture2D* src2d = dynamic_cast<FGraphicSubsystemDX11Texture2D*>(src);
		FGraphicSubsystemDX11Texture2D* dst2d = dynamic_cast<FGraphicSubsystemDX11Texture2D*>(dst);
		FGraphicSubsystemDX11Device* device11 = dynamic_cast<FGraphicSubsystemDX11Device*>(device);

		if (!src)
			throw "Source texture is NULL";
		if (!dst)
			throw "Destination texture is NULL";
		if (src2d->GetType() != EGraphicSubsystemTextureType::TEXTURE_2D || dst->GetType() != EGraphicSubsystemTextureType::TEXTURE_2D)
			throw "Source and destination textures must be a 2D textures";
		if (dst->GetColorFormat() !=src->GetColorFormat())
			throw "Source and destination formats do not match";

		/* apparently casting to the same type that the variable
		 * already exists as is supposed to prevent some warning
		 * when used with the conditional operator? */
		uint32_t copyWidth = (uint32_t)src_w ? (uint32_t)src_w
			: (src2d->GetWidth() - src_x);
		uint32_t copyHeight = (uint32_t)src_h ? (uint32_t)src_h
			: (src2d->GetHeight() - src_y);

		uint32_t dstWidth = dst2d->GetWidth() - dst_x;
		uint32_t dstHeight = dst2d->GetHeight() - dst_y;

		if (dstWidth < copyWidth || dstHeight < copyHeight)
			throw "Destination texture region is not big "
			"enough to hold the source region";

		if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 &&
			src_w == 0 && src_h == 0) {
			copyWidth = 0;
			copyHeight = 0;
		}

		device11->CopyTex(dst2d, dst_x, dst_y, src2d, src_x, src_y,
			copyWidth, copyHeight);

	}
	catch (_com_error error) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "device_copy_texture (D3D11): {}", (const char*)error.Description());
	}
	catch (const char* error) {
		SIMPLELOG_LOGGER_ERROR(nullptr, "device_copy_texture (D3D11): {}", error);
	}
}

bool FGraphicSubsystemDX11::TextureMap(FGraphicSubsystemTexture* _tex, uint8_t** ptr, uint32_t* linesize)
{
	auto tex2d = dynamic_cast<FGraphicSubsystemDX11Texture2D*>(_tex);
	HRESULT hr;

	if (tex2d->TextureType != EGraphicSubsystemTextureType::TEXTURE_2D)
		return false;

	D3D11_MAPPED_SUBRESOURCE map;
	hr = tex2d->GetDX11Device()->Context->Map(tex2d->Texture.Get(), 0,
		D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (FAILED(hr))
		return false;

	*ptr = (uint8_t*)map.pData;
	*linesize = map.RowPitch;
	return true;
}

bool FGraphicSubsystemDX11::TextureUnmap(FGraphicSubsystemTexture* tex)
{
	auto tex2d = dynamic_cast<FGraphicSubsystemDX11Texture2D*>(tex);
	if (tex2d->TextureType != EGraphicSubsystemTextureType::TEXTURE_2D)
		return false;

	tex2d->GetDX11Device()->Context->Unmap(tex2d->Texture.Get(), 0);
	return true;
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


