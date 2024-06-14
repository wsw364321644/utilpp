#include "Graphic/GraphicSubsystemDXGI.h"
#include <string_convert.h>
#include <comdef.h>	

#include <LoggerHelper.h>



static bool GetOutputDesc1(IDXGIOutput* const output, DXGI_OUTPUT_DESC1* desc1)
{
	ComPtr<IDXGIOutput6> output6;
	HRESULT hr = output->QueryInterface(IID_PPV_ARGS(&output6));
	bool success = SUCCEEDED(hr);
	if (success) {
		hr = output6->GetDesc1(desc1);
		success = SUCCEEDED(hr);
		if (!success) {
			SIMPLELOG_LOGGER_WARN(nullptr, "IDXGIOutput6::GetDesc1 failed: {}", hr);
		}
	}
	return success;
}

static bool GetMonitorTarget(const MONITORINFOEXW& info,
	DISPLAYCONFIG_TARGET_DEVICE_NAME& target)
{
	bool found = false;

	UINT32 numPath, numMode;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPath,
		&numMode) == ERROR_SUCCESS) {
		std::vector<DISPLAYCONFIG_PATH_INFO> paths(numPath);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(numMode);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPath,
			paths.data(), &numMode, modes.data(),
			nullptr) == ERROR_SUCCESS) {
			paths.resize(numPath);
			for (size_t i = 0; i < numPath; ++i) {
				const DISPLAYCONFIG_PATH_INFO& path = paths[i];

				DISPLAYCONFIG_SOURCE_DEVICE_NAME source;
				source.header.type =
					DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
				source.header.size = sizeof(source);
				source.header.adapterId =
					path.sourceInfo.adapterId;
				source.header.id = path.sourceInfo.id;
				if (DisplayConfigGetDeviceInfo(
					&source.header) == ERROR_SUCCESS &&
					wcscmp(info.szDevice,
						source.viewGdiDeviceName) == 0) {
					target.header.type =
						DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
					target.header.size = sizeof(target);
					target.header.adapterId =
						path.sourceInfo.adapterId;
					target.header.id = path.targetInfo.id;
					found = DisplayConfigGetDeviceInfo(
						&target.header) ==
						ERROR_SUCCESS;
					break;
				}
			}
		}
	}

	return found;
}


static bool IsInternalVideoOutput(
	const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType)
{
	switch (VideoOutputTechnologyType) {
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
		return TRUE;

	default:
		return FALSE;
	}
}



static HRESULT GetPathInfo(_In_ PCWSTR pszDeviceName,
	_Out_ DISPLAYCONFIG_PATH_INFO* pPathInfo)
{
	HRESULT hr = S_OK;
	UINT32 NumPathArrayElements = 0;
	UINT32 NumModeInfoArrayElements = 0;
	DISPLAYCONFIG_PATH_INFO* PathInfoArray = nullptr;
	DISPLAYCONFIG_MODE_INFO* ModeInfoArray = nullptr;

	do {
		// In case this isn't the first time through the loop, delete the buffers allocated
		delete[] PathInfoArray;
		PathInfoArray = nullptr;

		delete[] ModeInfoArray;
		ModeInfoArray = nullptr;

		hr = HRESULT_FROM_WIN32(GetDisplayConfigBufferSizes(
			QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements,
			&NumModeInfoArrayElements));
		if (FAILED(hr)) {
			break;
		}

		PathInfoArray = new (std::nothrow)
			DISPLAYCONFIG_PATH_INFO[NumPathArrayElements];
		if (PathInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		ModeInfoArray = new (std::nothrow)
			DISPLAYCONFIG_MODE_INFO[NumModeInfoArrayElements];
		if (ModeInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		hr = HRESULT_FROM_WIN32(QueryDisplayConfig(
			QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements,
			PathInfoArray, &NumModeInfoArrayElements, ModeInfoArray,
			nullptr));
	} while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

	INT DesiredPathIdx = -1;

	if (SUCCEEDED(hr)) {
		// Loop through all sources until the one which matches the 'monitor' is found.
		for (UINT PathIdx = 0; PathIdx < NumPathArrayElements;
			++PathIdx) {
			DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
			SourceName.header.type =
				DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			SourceName.header.size = sizeof(SourceName);
			SourceName.header.adapterId =
				PathInfoArray[PathIdx].sourceInfo.adapterId;
			SourceName.header.id =
				PathInfoArray[PathIdx].sourceInfo.id;

			hr = HRESULT_FROM_WIN32(
				DisplayConfigGetDeviceInfo(&SourceName.header));
			if (SUCCEEDED(hr)) {
				if (wcscmp(pszDeviceName,
					SourceName.viewGdiDeviceName) == 0) {
					// Found the source which matches this hmonitor. The paths are given in path-priority order
					// so the first found is the most desired, unless we later find an internal.
					if (DesiredPathIdx == -1 ||
						IsInternalVideoOutput(
							PathInfoArray[PathIdx]
							.targetInfo
							.outputTechnology)) {
						DesiredPathIdx = PathIdx;
					}
				}
			}
		}
	}

	if (DesiredPathIdx != -1) {
		*pPathInfo = PathInfoArray[DesiredPathIdx];
	}
	else {
		hr = E_INVALIDARG;
	}

	delete[] PathInfoArray;
	PathInfoArray = nullptr;

	delete[] ModeInfoArray;
	ModeInfoArray = nullptr;

	return hr;
}



static HRESULT GetPathInfo(HMONITOR hMonitor,
	_Out_ DISPLAYCONFIG_PATH_INFO* pPathInfo)
{
	HRESULT hr = S_OK;

	// Get the name of the 'monitor' being requested
	MONITORINFOEXW ViewInfo;
	RtlZeroMemory(&ViewInfo, sizeof(ViewInfo));
	ViewInfo.cbSize = sizeof(ViewInfo);
	if (!GetMonitorInfoW(hMonitor, &ViewInfo)) {
		// Error condition, likely invalid monitor handle, could log error
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	if (SUCCEEDED(hr)) {
		hr = GetPathInfo(ViewInfo.szDevice, pPathInfo);
	}

	return hr;
}
static ULONG GetSdrWhiteNits(HMONITOR monitor)
{
	ULONG nits = 0;

	DISPLAYCONFIG_PATH_INFO info;
	if (SUCCEEDED(GetPathInfo(monitor, &info))) {
		const DISPLAYCONFIG_PATH_TARGET_INFO& targetInfo =
			info.targetInfo;

		DISPLAYCONFIG_SDR_WHITE_LEVEL level;
		DISPLAYCONFIG_DEVICE_INFO_HEADER& header = level.header;
		header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
		header.size = sizeof(level);
		header.adapterId = targetInfo.adapterId;
		header.id = targetInfo.id;
		if (DisplayConfigGetDeviceInfo(&header) == ERROR_SUCCESS)
			nits = (level.SDRWhiteLevel * 80) / 1000;
	}

	return nits;
}
static inline void LogAdapterMonitors(IDXGIAdapter1* adapter)
{
	UINT i;
	ComPtr<IDXGIOutput> output;

	for (i = 0; adapter->EnumOutputs(i, &output) == S_OK; ++i) {
		DXGI_OUTPUT_DESC desc;
		if (FAILED(output->GetDesc(&desc)))
			continue;

		unsigned refresh = 0;

		bool target_found = false;
		DISPLAYCONFIG_TARGET_DEVICE_NAME target;

		MONITORINFOEXW info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfoW(desc.Monitor, &info)) {
			target_found = GetMonitorTarget(info, target);

			DEVMODEW mode;
			mode.dmSize = sizeof(mode);
			mode.dmDriverExtra = 0;
			if (EnumDisplaySettingsW(info.szDevice,
				ENUM_CURRENT_SETTINGS, &mode)) {
				refresh = mode.dmDisplayFrequency;
			}
		}

		if (!target_found) {
			target.monitorFriendlyDeviceName[0] = 0;
		}

		DXGI_COLOR_SPACE_TYPE type =
			DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		FLOAT min_luminance = 0.f;
		FLOAT max_luminance = 0.f;
		FLOAT max_full_frame_luminance = 0.f;
		DXGI_OUTPUT_DESC1 desc1;
		if (GetOutputDesc1(output.Get(), &desc1)) {
			type = desc1.ColorSpace;
			min_luminance = desc1.MinLuminance;
			max_luminance = desc1.MaxLuminance;
			max_full_frame_luminance = desc1.MaxFullFrameLuminance;
		}

		const char* space = "Unknown";
		switch (type) {
		case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
			space = "RGB_FULL_G22_NONE_P709";
			break;
		case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
			space = "RGB_FULL_G2084_NONE_P2020";
			break;
		default:
			SIMPLELOG_LOGGER_WARN(nullptr, "Unexpected DXGI_COLOR_SPACE_TYPE: {}", (unsigned)type);
		}

		const RECT& rect = desc.DesktopCoordinates;
		const ULONG nits = GetSdrWhiteNits(desc.Monitor);
		auto monitorFriendlyDeviceNameu8 = U16ToU8((const char16_t*)target.monitorFriendlyDeviceName);
		SIMPLELOG_LOGGER_INFO(nullptr,
			"\t  output {}:\n"
			"\t    name={}\n"
			"\t    pos=[{}, {}]\n"
			"\t    size=[{}, {}]\n"
			"\t    attached={}\n"
			"\t    refresh={}\n"
			"\t    space={}\n"
			"\t    sdr_white_nits={}\n"
			"\t    nit_range=[min=%{}, max={}, max_full_frame={}]",
			i, monitorFriendlyDeviceNameu8, rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			desc.AttachedToDesktop ? "true" : "false", refresh, space,
			nits, min_luminance, max_luminance,
			max_full_frame_luminance);
	}
}

void LogD3DAdapters()
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw _com_error(hr);

	for (i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		auto str = U16ToU8((const char16_t*)desc.Description);
		SIMPLELOG_LOGGER_INFO(nullptr, "Adapter {}: {}", i, str.c_str());
		SIMPLELOG_LOGGER_INFO(nullptr, "Dedicated VRAM:  {}", desc.DedicatedVideoMemory);
		SIMPLELOG_LOGGER_INFO(nullptr, "Shared VRAM:   {}", desc.SharedSystemMemory);
		SIMPLELOG_LOGGER_INFO(nullptr, " PCI ID:         {}:{}", desc.VendorId, desc.DeviceId);

		/* driver version */
		LARGE_INTEGER umd;
		hr = adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),
			&umd);
		if (SUCCEEDED(hr)) {
			const uint64_t version = umd.QuadPart;
			const uint16_t aa = (version >> 48) & 0xffff;
			const uint16_t bb = (version >> 32) & 0xffff;
			const uint16_t ccccc = (version >> 16) & 0xffff;
			const uint16_t ddddd = version & 0xffff;
			SIMPLELOG_LOGGER_INFO(nullptr, "Driver Version: {}.{}.{}.{}", aa, bb, ccccc, ddddd);
		}
		else {
			SIMPLELOG_LOGGER_INFO(nullptr, "Driver Version: Unknown {}", (unsigned)hr);
		}

		LogAdapterMonitors(adapter.Get());
	}
}




static inline void EnumD3DAdapters(bool (*callback)(void*, const char*, uint32_t), void* param)
{
	Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw _com_error(hr);

	for (i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		auto str = U16ToU8((const char16_t*)desc.Description);

		if (!callback(param, str.c_str(), i))
			break;
	}
}


FGraphicSubsystemDXGIDevice::FGraphicSubsystemDXGIDevice(uint32_t adapterIdx):AdpIdx(adapterIdx)
{
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
	if (FAILED(hr))
		throw _com_error( hr);

	hr = Factory->EnumAdapters1(adapterIdx, &Adapter);
	if (FAILED(hr))
		throw _com_error( hr);
}


EGraphicSubsystemError FGraphicSubsystemDXGI::DeviceEnumAdapters(bool(*callback)(void* param, const char* name, uint32_t id), void* param)
{
	try {
		EnumD3DAdapters(callback, param);
		return EGraphicSubsystemError::OK;
	}
	catch (const _com_error& error) {
		SIMPLELOG_LOGGER_WARN(nullptr, "Failed enumerating devices: {} ({})", error.ErrorMessage(), error.Error());

		return EGraphicSubsystemError::DXGIFailed;
	}
}


