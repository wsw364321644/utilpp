#include "os_info_helper.h"
#include <simple_os_defs.h>

/**
* https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-element-size-limits
*/
#define REG_KEY_MAX ((1<<8)-1)
#define REG_VALUE_MAX ((1<<14)-1)

bool IsOS64Bit() { 
    if (IsProcess64Bit()) {
        return true;
    }
    else {
        USHORT ProcessMachine=0, NativeMachine=0;
        bool bres;
        bres =IsWow64Process2(GetCurrentProcess(), &ProcessMachine, &NativeMachine);
        if (!bres) {
            return false;
        }
        if (ProcessMachine== IMAGE_FILE_MACHINE_UNKNOWN) {
            return true;
        }
    }
    return false;
}
bool IsProcess64Bit() {
#if defined(_WIN64)
    return true;
#else
    return false;
#endif
}


void GetOsInfo(OSInfo_t* pinfo)
{
    OSInfo_t& info = *pinfo;
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

    if (dwMajorVersion == 5) {
        info.OSCoreType = EOSCoreType::OSCT_WinXP;
        strcpy(info.OSName, "windows XP");
    }
    else if (dwMajorVersion == 6 && dwMinorVersion == 0) {
        info.OSCoreType = EOSCoreType::OSCT_WinVista;
        strcpy(info.OSName, "windows vista");
    }
    else if (dwMajorVersion == 6 && dwMinorVersion == 1) {
        info.OSCoreType = EOSCoreType::OSCT_Windows7;
        strcpy(info.OSName, "windows 7");
    }
    else if (dwMajorVersion == 6 && dwMinorVersion == 2) {
        info.OSCoreType = EOSCoreType::OSCT_Windows8;
        strcpy(info.OSName, "windows 8");
    }
    else if (dwMajorVersion == 6 && dwMinorVersion == 3) {
        info.OSCoreType = EOSCoreType::OSCT_Windows81;
        strcpy(info.OSName, "windows 8.1");

    }
    else if (dwMajorVersion == 10) {
        if (dwBuildNumber >= 22000) {
            info.OSCoreType = EOSCoreType::OSCT_Win11;
            strcpy(info.OSName, "windows 11");
        }
        else if (dwBuildNumber >= 20348) {
            info.OSCoreType = EOSCoreType::OSCT_Win2022;
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
        }
        else
            info.OSCoreType = EOSCoreType::OSCT_Windows10;
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
        char* LeftBraceCursor = NULL;
        char* RightBraceCursor = NULL;
        for (int i = 0; i < strlen(hwProfileInfo.szHwProfileGuid); i++) {
            if (hwProfileInfo.szHwProfileGuid[i] == '}') {
                RightBraceCursor = hwProfileInfo.szHwProfileGuid + i;
                break;
            }
            if (hwProfileInfo.szHwProfileGuid[i] == '{') {
                LeftBraceCursor = hwProfileInfo.szHwProfileGuid + i;
            }
        }
        if (LeftBraceCursor && RightBraceCursor) {
            memcpy(info.DeviceGUID, LeftBraceCursor + 1, RightBraceCursor - LeftBraceCursor - 1);
            info.DeviceGUID[RightBraceCursor - LeftBraceCursor - 1] = '\0';
        }
    }
    return;
}


/**
* https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys
*/
bool GetMachineUUID(char* buf, uint32_t* buflen, bool* isGUID) {
    if (!buflen) {
        return false;
    }
    if (*buflen <= MACHINE_GUID_MAX) {
        *buflen = MACHINE_GUID_MAX+1;
        if (buf) {
            return false;
        }
        else {
            return true;
        }
    }
    if (isGUID) {
        *isGUID = true;
    }
    HKEY hKey;
    LSTATUS res;
    DWORD len = *buflen;
    DWORD dType;

    res=RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ| KEY_ENUMERATE_SUB_KEYS| KEY_WOW64_64KEY, &hKey);
    if (res != ERROR_SUCCESS) {
        return false;
    }
    dType = REG_SZ;
    res =RegGetValueA(hKey, NULL, "MachineGuid", RRF_RT_ANY, NULL, buf, &len);
    if (res == ERROR_SUCCESS) {
        *buflen = len;
        return true;
    }
    else {
        return false;
    }
}