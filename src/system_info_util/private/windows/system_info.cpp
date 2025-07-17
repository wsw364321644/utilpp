#include "system_info.h"

#include <Windows.h>

#include <strsafe.h>
#include <intrin.h>
#include <string>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <libcpuid.h>
#include <LoggerHelper.h>
#include <string_convert.h>

#include <functional>
#define _WIN32_DCOM

#pragma comment(lib, "wbemuuid.lib")
namespace utilpp {
#define MB (1024*1024)

    typedef std::function<void(IWbemServices*)> fnQuery;
    bool QueryWMI(fnQuery fn) {
        HRESULT hres;

        // Initialize COM.
        hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        if (FAILED(hres))
        {
            SIMPLELOG_LOGGER_ERROR(nullptr, "Failed to initialize COM library.Error code ={} ", hres);
            return false;              // Program has failed.
        }

        //// Initialize 
        //hres = CoInitializeSecurity(
        //	NULL,
        //	-1,      // COM negotiates service                  
        //	NULL,    // Authentication services
        //	NULL,    // Reserved
        //	RPC_C_AUTHN_LEVEL_DEFAULT,    // authentication
        //	RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation
        //	NULL,             // Authentication info 
        //	EOAC_NONE,        // Additional capabilities
        //	NULL              // Reserved
        //);


        //if (FAILED(hres))
        //{
        //	SIMPLELOG_LOGGER_ERROR(nullptr,"Failed to initialize security. Error code ={} ", hres);
        //	CoUninitialize();
        //	return false;           // Program has failed.
        //}


        // Obtain the initial locator to Windows Management
        // on a particular host computer.
        IWbemLocator* pLoc = 0;

        hres = CoCreateInstance(
            CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLoc);

        if (FAILED(hres) || !pLoc)
        {
            SIMPLELOG_LOGGER_ERROR(nullptr, "Failed to create IWbemLocator object. Error code ={} ", hres);
            CoUninitialize();
            return false;        // Program has failed.
        }

        IWbemServices* pSvc = 0;

        // Connect to the root\cimv2 namespace with the
        // current user and obtain pointer pSvc
        // to make IWbemServices calls.

        hres = pLoc->ConnectServer(
            (const BSTR)OLESTR("ROOT\\CIMV2"), // WMI namespace
            NULL,                    // User name
            NULL,                    // User password
            0,                       // Locale
            NULL,                    // Security flags                 
            0,                       // Authority       
            0,                       // Context object
            &pSvc                    // IWbemServices proxy
        );

        if (FAILED(hres))
        {
            SIMPLELOG_LOGGER_ERROR(nullptr, "Could not connect. Error code ={} ", hres);

            pLoc->Release();
            CoUninitialize();
            return false;                // Program has failed.
        }
        SIMPLELOG_LOGGER_DEBUG(nullptr, "Connected to ROOT\\CIMV2 WMI namespace");

        // Set the IWbemServices proxy so that impersonation
        // of the user (client) occurs.
        hres = CoSetProxyBlanket(

            pSvc,                         // the proxy to set
            RPC_C_AUTHN_WINNT,            // authentication service
            RPC_C_AUTHZ_NONE,             // authorization service
            NULL,                         // Server principal name
            RPC_C_AUTHN_LEVEL_CALL,       // authentication level
            RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
            NULL,                         // client identity 
            EOAC_NONE                     // proxy capabilities     
        );

        if (FAILED(hres))
        {
            SIMPLELOG_LOGGER_ERROR(nullptr, "Could not set proxy blanket. Error code ={} ", hres);

            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return false;              // Program has failed.
        }


        // Use the IWbemServices pointer to make requests of WMI. 
        // Make requests here:
        fn(pSvc);

        // Cleanup
        // ========

        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        //system("pause");
        return true;
    }

    // 获取内存信息
    void GetMemStatus(MemInfo_t& info)
    {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        info.TotalBytes = statex.ullTotalPhys;
        info.MemoryLoad = statex.dwMemoryLoad;
        info.Num = 0;
        QueryWMI([&](IWbemServices* pSvc) {
            {
                HRESULT hres;
                // For example, query for all the running processes
                IEnumWbemClassObject* pEnumerator = NULL;
                hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_PhysicalMemory"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

                if (FAILED(hres))
                {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Query for Win32_PhysicalMemory failed. Error code ={} ", hres);
                    return;               // Program has failed.
                }

                IWbemClassObject* pclsObj;
                ULONG uReturn = 0;
                for (int i = 0; i < DRIVER_NUM_MAX && pEnumerator; i++) {
                    hres = pEnumerator->Next(WBEM_INFINITE, 1,
                        &pclsObj, &uReturn);
                    if (0 == uReturn)
                    {
                        info.Num = i;
                        break;
                    }

                    VARIANT vtProp{};

                    // Get the value of the Name property
                    hres = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres)) {
                        auto desstr = U16ToU8((const char16_t*)vtProp.bstrVal,GetStringLengthW(vtProp.bstrVal));
                        strcpy(info.PhysicalMemoryInfos[i].Manufacturer, desstr.c_str());
                        VariantClear(&vtProp);
                    }

                    hres = pclsObj->Get(L"Speed", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres)) {
                        info.PhysicalMemoryInfos[i].Speed = vtProp.uintVal;
                        VariantClear(&vtProp);
                    }

                    hres = pclsObj->Get(L"SMBIOSMemoryType", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres)) {
                        info.PhysicalMemoryInfos[i].SMBIOSMemoryType = vtProp.uintVal;
                        VariantClear(&vtProp);
                    }

                    hres = pclsObj->Get(L"Capacity", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres)) {
                        info.PhysicalMemoryInfos[i].TotalBytes = std::stoull(vtProp.bstrVal);
                        VariantClear(&vtProp);
                    }
                }
            }
            });
    }


    void GetDiskInfos(DriverInfos_t& infos)
    {
        //-------------------------------------------------------------------//  
        //通过GetLogicalDriveStrings()函数获取所有驱动器字符串信息长度  
        int DSLength = GetLogicalDriveStringsA(0, NULL);
        infos.NumLogical = DSLength / 4;
        auto num = infos.NumLogical;

        char* DStr = new char[DSLength];
        memset(DStr, 0, DSLength);

        //通过GetLogicalDriveStrings将字符串信息复制到堆区数组中,其中保存了所有驱动器的信息。  
        GetLogicalDriveStringsA(DSLength, DStr);
        int DType;
        BOOL fResult;
        unsigned _int64 i64FreeBytesToCaller;
        unsigned _int64 i64TotalBytes;
        unsigned _int64 i64FreeBytes;

        //读取各驱动器信息，由于DStr内部数据格式是A:\NULLB:\NULLC:\NULL，所以DSLength/4可以获得具体大循环范围  
        for (int i = 0; i < num; ++i)
        {

            auto& DriverInfo = infos.LogicalDrivers[i];
            std::string strdriver = DStr + i * 4;
            std::string strTmp, strTotalBytes, strFreeBytes;
            DType = GetDriveTypeA(strdriver.c_str());//GetDriveType函数，可以获取驱动器类型，参数为驱动器的根目录  
            switch (DType)
            {
            case DRIVE_FIXED: {
                DriverInfo.DriverType = EDriverType::E_DRIVE_FIXED;
            }
                            break;
            case DRIVE_CDROM: {
                DriverInfo.DriverType = EDriverType::E_DRIVE_CDROM;
            }
                            break;
            case DRIVE_REMOVABLE: {
                DriverInfo.DriverType = EDriverType::E_DRIVE_REMOVABLE;
            }
                                break;
            case DRIVE_REMOTE: {
                DriverInfo.DriverType = EDriverType::E_DRIVE_REMOTE;
            }
                             break;
            case DRIVE_RAMDISK: {
                DriverInfo.DriverType = EDriverType::E_DRIVE_RAMDISK;
            }
                              break;
            default:
                DriverInfo.DriverType = EDriverType::E_DRIVE_UNKNOWN;
                break;
            }

            //GetDiskFreeSpaceEx函数，可以获取驱动器磁盘的空间状态,函数返回的是个BOOL类型数据  
            fResult = GetDiskFreeSpaceExA(strdriver.c_str(),
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&i64TotalBytes,
                (PULARGE_INTEGER)&i64FreeBytes);

            if (fResult) {
                DriverInfo.FreeBytes = i64FreeBytes;
                DriverInfo.FreeBytesToCaller = i64FreeBytesToCaller;
                DriverInfo.TotalBytes = i64TotalBytes;
            }
        }
        delete[] DStr;




        infos.Num = 0;
        QueryWMI([&](IWbemServices* pSvc) {
            {
                HRESULT hres;
                // For example, query for all the running processes
                IEnumWbemClassObject* pEnumerator = NULL;
                hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_DiskDrive "),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

                if (FAILED(hres))
                {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Query for Win32_DiskDrive  failed. Error code ={} ", hres);
                    return;               // Program has failed.
                }

                IWbemClassObject* pclsObj;
                ULONG uReturn = 0;
                for (int i = 0; i < DRIVER_NUM_MAX && pEnumerator; i++) {
                    hres = pEnumerator->Next(WBEM_INFINITE, 1,
                        &pclsObj, &uReturn);
                    if (0 == uReturn)
                    {
                        infos.Num = i;
                        break;
                    }

                    VARIANT vtProp{};

                    // Get the value of the Name property
                    hres = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
                        auto desstr = U16ToU8((const char16_t*)vtProp.bstrVal,GetStringLengthW(vtProp.bstrVal));
                        strcpy(infos.Drivers[i].Model, desstr.c_str());
                        VariantClear(&vtProp);
                    }

                    hres = pclsObj->Get(L"Size", 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
                        auto desstr = U16ToU8((const char16_t*)vtProp.bstrVal,GetStringLengthW(vtProp.bstrVal));
                        infos.Drivers[i].TotalBytes = std::strtoull(desstr.c_str(), NULL, 0);
                        VariantClear(&vtProp);
                    }
                }
            }
            });



        return;
    }




    void GetOsInfo(OSInfo_t& info)
    {
        // get os name according to version number
        //OSVERSIONINFO osver = { sizeof(OSVERSIONINFO) };
        //GetVersionEx(&osver);

        //if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0)
        //	strcpy(info.OSName, "windows vista");
        //else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1)
        //	strcpy(info.OSName, "windows 7");
        //else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
        //	strcpy(info.OSName, "windows 8");
        //else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3)
        //	strcpy(info.OSName, "windows 8.1");
        //else if (osver.dwMajorVersion == 10)
        //	strcpy(info.OSName, "windows 10");
        //else if (osver.dwMajorVersion == 11)
        //	strcpy(info.OSName, "windows 11");
        //else
        //	strcpy(info.OSName, "unknow");

        typedef LONG(__stdcall* fnRtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);
        fnRtlGetVersion pRtlGetVersion;
        HMODULE hNtdll;
        LONG ntStatus;
        ULONG    dwMajorVersion = 0;
        ULONG    dwMinorVersion = 0;
        ULONG    dwBuildNumber = 0;
        RTL_OSVERSIONINFOW VersionInformation = { 0 };


        hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll == NULL)return;

        pRtlGetVersion = (fnRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
        if (pRtlGetVersion == NULL)return;

        VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
        ntStatus = pRtlGetVersion(&VersionInformation);

        if (ntStatus != 0)return;

        dwMajorVersion = VersionInformation.dwMajorVersion;
        dwMinorVersion = VersionInformation.dwMinorVersion;
        dwBuildNumber = VersionInformation.dwBuildNumber;

        if (dwMajorVersion == 5)
            strcpy(info.OSName, "windows XP");
        else if (dwMajorVersion == 6 && dwMinorVersion == 0)
            strcpy(info.OSName, "windows vista");
        else if (dwMajorVersion == 6 && dwMinorVersion == 1)
            strcpy(info.OSName, "windows 7");
        else if (dwMajorVersion == 6 && dwMinorVersion == 2)
            strcpy(info.OSName, "windows 8");
        else if (dwMajorVersion == 6 && dwMinorVersion == 3)
            strcpy(info.OSName, "windows 8.1");

        else if (dwMajorVersion == 10) {
            if (dwBuildNumber >= 22000)
                strcpy(info.OSName, "windows 11");
            else if (dwBuildNumber >= 20348)
                strcpy(info.OSName, "SERVER 2022");
            //else if (dwBuildNumber >= 19044)
            //	strcpy(info.OSName, "WIN10 21H2");
            //else if (dwBuildNumber >= 19043)
            //	strcpy(info.OSName, "WIN10 20H2");
            //else if (dwBuildNumber >= 19042)
            //	strcpy(info.OSName, "WIN10 20H1");
            //else if (dwBuildNumber >= 19041)
            //	strcpy(info.OSName, "WIN10 H2");
            //else if (dwBuildNumber >= 18363)
            //else if (dwBuildNumber >= 18362)
            //else if (dwBuildNumber >= 17763)
            //else if (dwBuildNumber >= 17134)
            //else if (dwBuildNumber >= 16299)
            //else if (dwBuildNumber >= 15063)
            //else if (dwBuildNumber >= 14393)
            //else if (dwBuildNumber >= 10586)
            else
                strcpy(info.OSName, "windows 10");
        }


        SYSTEM_INFO sysinfo;
        GetNativeSystemInfo(&sysinfo);
        if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
            strcpy(info.OSName + strlen(info.OSName), " X64");
        }
        else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
            strcpy(info.OSName + strlen(info.OSName), " X86");
        }

        HW_PROFILE_INFOA hwProfileInfo;
        if (GetCurrentHwProfileA(&hwProfileInfo)) {
            std::string guid(hwProfileInfo.szHwProfileGuid);
            guid = guid.substr(guid.find('{') + 1, guid.find('}') - guid.find('{') - 1);
            strcpy(info.DeviceGUID, guid.c_str());
        }
        return;

    }

    // CPU 信息
    void GetCpuInfoPlat(CpuInfo_t& info)
    {
        int CPUInfo[4] = { -1 };
        unsigned   nExIds, i = 0;


        // Get the information associated with each extended ID.
        __cpuid(CPUInfo, 0x80000000);
        nExIds = CPUInfo[0];
        for (i = 0x80000000; i <= nExIds; ++i)
        {
            __cpuid(CPUInfo, i);
            // Interpret CPU brand string
            if (i == 0x80000002)
                memcpy(info.ProcessorBrandString, CPUInfo, sizeof(CPUInfo));
            else if (i == 0x80000003)
                memcpy(info.ProcessorBrandString + sizeof(CPUInfo), CPUInfo, sizeof(CPUInfo));
            else if (i == 0x80000004)
                memcpy(info.ProcessorBrandString + sizeof(CPUInfo) * 2, CPUInfo, sizeof(CPUInfo));
        }

        __cpuid(CPUInfo, 0);
        memcpy(info.IdentificationString, CPUInfo + 1, sizeof(int));
        memcpy(info.IdentificationString + sizeof(int), CPUInfo + 3, sizeof(int));
        memcpy(info.IdentificationString + sizeof(int) * 2, CPUInfo + 2, sizeof(int));
        if (CPUInfo[0] >= 0x16) {
            __cpuid(CPUInfo, 0x16);
            info.ProcessorBaseFrequencyMHz = CPUInfo[0];
            info.MaximumFrequencyMHz = CPUInfo[1];
            info.BusFrequencyMHz = CPUInfo[0];
        }

        __cpuid(CPUInfo, 1);
        info.LogicalCoreNum = (CPUInfo[1] >> 16) & 0xff; // EBX[23:16]

        //SYSTEM_INFO sysInfo;
        //GetSystemInfo(&sysInfo);
        //std::cout << "Number of Cores: " << sysInfo.dwNumberOfProcessors << std::endl;



    }


    BOOL GetDisplayMonitorInfo(int nDeviceIndex, LPSTR lpszMonitorInfo)
    {
        typedef BOOL(WINAPI* PEnumDisplayDevicesA)(_In_opt_ LPCSTR lpDevice, _In_ DWORD iDevNum, _Inout_ PDISPLAY_DEVICEA lpDisplayDevice, _In_ DWORD dwFlags);
        PEnumDisplayDevicesA pEnumDisplayDevices;

        HINSTANCE  hInstUser32;
        DISPLAY_DEVICE DispDev;
        char szSaveDeviceName[33];  // 32 + 1 for the null-terminator 
        BOOL bRet = TRUE;
        HRESULT hr;

        hInstUser32 = GetModuleHandleA("user32.dll");
        if (!hInstUser32) return FALSE;

        // Get the address of the EnumDisplayDevices function 
        pEnumDisplayDevices = (PEnumDisplayDevicesA)GetProcAddress(hInstUser32, "EnumDisplayDevicesA");
        if (!pEnumDisplayDevices) {
            FreeLibrary(hInstUser32);
            return FALSE;
        }

        ZeroMemory(&DispDev, sizeof(DispDev));
        DispDev.cb = sizeof(DispDev);

        // After the first call to EnumDisplayDevices,  
        // DispDev.DeviceString is the adapter name 
        if (pEnumDisplayDevices(NULL, nDeviceIndex, &DispDev, 0))
        {
            hr = StringCchCopyA(szSaveDeviceName, 33, DispDev.DeviceName);
            if (FAILED(hr))
            {
                // TODO: write error handler 
            }

            // After second call, DispDev.DeviceString is the  
            // monitor name for that device  
            if (pEnumDisplayDevices(szSaveDeviceName, 0, &DispDev, 0)) {
                // In the following, lpszMonitorInfo must be 128 + 1 for  
                // the null-terminator. 
                hr = StringCchCopyA(lpszMonitorInfo, 129, DispDev.DeviceID);
                if (FAILED(hr))
                {
                    // TODO: write error handler 
                }
            }
            else {
                bRet = false;
            }
        }
        else {
            bRet = FALSE;
        }

        FreeLibrary(hInstUser32);

        return bRet;
    }

    void GetDisplayInfos(DisplayInfos_t& info) {
        bool res = false;
        info.Num = 0;
        for (int i = 0; i < DISPLAY_NUM_MAX; i++) {
            res = GetDisplayMonitorInfo(i, info.Displays[i].DisplayName);
            if (!res) {
                info.Num = i;
                break;
            }
        }
    }

    void GetBIOSInfo(BIOSInfo_t& info) {
        HKEY hKey;
        DWORD dType = REG_SZ;
        LSTATUS result;

        uint32_t buflen = 0;

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\BIOS)", NULL, KEY_READ, &hKey) == 0)
        {
            buflen = SYS_PRODUCT_MAX;
            result = RegQueryValueExA(hKey, "BaseBoardProduct", NULL, &dType, (LPBYTE)info.BaseBoardProduct, (LPDWORD)&buflen);
            if (result != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return;
            }

            buflen = SYS_PRODUCT_MAX;
            result = RegQueryValueExA(hKey, "BaseBoardManufacturer", NULL, &dType, (LPBYTE)info.BaseBoardManufacturer, (LPDWORD)&buflen);
            if (result != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return;
            }

            RegCloseKey(hKey);
        }
        else {
            return;
        }
        return;
    }

    void GetVideoControllerInfos(VideoControllerInfos_t& info) {
        info.Num = 0;
        QueryWMI([&](IWbemServices* pSvc) {
            {
                HRESULT hres;
                // For example, query for all the running processes
                IEnumWbemClassObject* pEnumerator = NULL;
                hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_VideoController"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

                if (FAILED(hres))
                {
                    SIMPLELOG_LOGGER_ERROR(nullptr, "Query for Win32_VideoController failed. Error code ={} ", hres);
                    return;               // Program has failed.
                }

                IWbemClassObject* pclsObj;
                ULONG uReturn = 0;
                for (int i = 0; i < DRIVER_NUM_MAX && pEnumerator; i++) {
                    hres = pEnumerator->Next(WBEM_INFINITE, 1,
                        &pclsObj, &uReturn);
                    if (0 == uReturn)
                    {
                        info.Num = i;
                        break;
                    }

                    VARIANT vtProp{};

                    // Get the value of the Name property
                    hres = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                    auto desstr = U16ToU8((const char16_t*)vtProp.bstrVal,GetStringLengthW(vtProp.bstrVal));
                    strcpy(info.Controllers[i].Name, desstr.c_str());
                    VariantClear(&vtProp);

                    hres = pclsObj->Get(L"Availability", 0, &vtProp, 0, 0);
                    SIMPLELOG_LOGGER_DEBUG(nullptr, "Availability: {} ", vtProp.uintVal);
                    info.Controllers[i].Availability = vtProp.uintVal;
                    VariantClear(&vtProp);
                }
            }
            });
    }
}