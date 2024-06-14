#include "Graphic/GraphicSubsystemDX9.h"
#include <string_convert.h>
#include <comdef.h>	
#include <d3d9.h>
#include <LoggerHelper.h>
#include <wrl.h>
using namespace Microsoft::WRL;
class FGraphicSubsystemDX9Device :public FGraphicSubsystemDevice {
public:
	FGraphicSubsystemDX9Device(uint32_t adapterIdx);
	ComPtr<IDirect3D9Ex> d3d9ex;
	ComPtr<IDirect3DDevice9Ex> d3ddev;
};

FGraphicSubsystemDX9Device::FGraphicSubsystemDX9Device(uint32_t adapterIdx)
{
	HRESULT hr;
	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
	if (FAILED(hr)) {
		throw _com_error(hr);
	}
	D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information
	ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
	d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	d3dpp.hDeviceWindow = NULL;    // set the window to be used by Direct3D

	//D3DDISPLAYMODEEX d3ddm;
	//ZeroMemory(&d3ddm, sizeof(d3ddm));
	//d3ddm.Format= D3DFORMAT::
	// 
	// create a device class using this information and information from the d3dpp stuct
	d3d9ex->CreateDeviceEx(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		NULL,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		NULL,
		&d3ddev);
}
const char* FGraphicSubsystemDX9::DeviceGetName(void)
{
    return "Direct3D 9";
}

EGraphicSubsystem FGraphicSubsystemDX9::DeviceGetType(void)
{
    return EGraphicSubsystem::DX9;
}

EGraphicSubsystemError FGraphicSubsystemDX9::DeviceEnumAdapters(bool(*callback)(void* param, const char* name, uint32_t id), void* param)
{
	return EGraphicSubsystemError();
}



EGraphicSubsystemError FGraphicSubsystemDX9::DeviceCreate(FGraphicSubsystemDevice** p_device, uint32_t adapter)
{
	FGraphicSubsystemDX9Device* device = NULL;
	EGraphicSubsystemError res = EGraphicSubsystemError::OK;
	try {

		device = new FGraphicSubsystemDX9Device(adapter);

	}catch(...) {

	}

	*p_device = device;
	return res;
}


