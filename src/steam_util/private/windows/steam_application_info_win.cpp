#include "steam_application_info.h"
#include <windows_helper.h>
#include <simple_os_process.h>
#include <string_convert.h>
#include <FunctionExitHelper.h>
#include <service_helper.h>
#include <shellapi.h>
#include <filesystem>
#include <regex>

constexpr char STEAM_EXE_NAME[] = "steam.exe";

bool IsSteamClientInstalled()
{
    HKEY hKey;
    std::filesystem::path path(STEAM_PRODUCT_NAME);
    path /= REG_COMMAND_PATH;
    LSTATUS res = RegOpenKeyExW(HKEY_CLASSES_ROOT, (LPCWSTR)path.u16string().c_str(), NULL, KEY_READ, &hKey);
    if (res == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    else if (res == ERROR_FILE_NOT_FOUND)
    {
        return false;
    }
    return true;
}

bool IsSteamClientStarted() {
    return IsServiceRunning(STEAM_SERVICE_NAME);
}

bool GetSteamClientPath(char* pathBuf, uint32_t* length) {
    if (!length) {
        return false;
    }
    HKEY hKey;
    std::filesystem::path path(STEAM_PRODUCT_NAME);
    path /= REG_COMMAND_PATH;
    DWORD dType = REG_SZ;
    LSTATUS result;
    uint32_t inbuflen = *length;
    *length = 0;
    uint32_t buflen = 0;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, (LPCWSTR)path.u16string().c_str(), NULL, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        FunctionExitHelper_t exitHelper(
            [&]() {
                RegCloseKey(hKey);
            }
        );
        result = RegQueryValueExW(hKey, NULL, NULL, &dType, NULL, (LPDWORD)&buflen);
        if (result != ERROR_SUCCESS) {
            return false;
        }
        LPBYTE buf;
        buf = (LPBYTE)malloc(buflen);
        result = RegQueryValueExW(hKey, NULL, NULL, &dType, buf, (LPDWORD)&buflen);
        if (result != ERROR_SUCCESS) {
            return false;
        }
        std::u16string rawstr((char16_t*)buf);
        rawstr = rawstr.substr(rawstr.find(u"\"") + 1, rawstr.find(u"\"", rawstr.find(u"\"") + 1) - 1);
        auto str = U16ToU8(rawstr.c_str(), rawstr.length());
        free(buf);

        *length = str.size() + 1;
        if (*length > inbuflen) {
            return false;
        }
        memcpy(pathBuf, str.c_str(), *length);
    }
    else {
        return false;
    }
    return  true;
}

void CloseSteamClient()
{
    iterate_process(
        [](ProcessInfo_t& info) {
            if (strstr(info.PathStr, STEAM_EXE_NAME)) {
                kill_process(info.Pid, 0);
            }
        }
    );
}

bool SetSteamAutoLoginUser(std::u8string_view name)
{
    HKEY hKey;
    std::filesystem::path path("Software\\Valve\\Steam");
    auto nameu16 = U8ToU16(name);
    if (RegCreateKeyExW(HKEY_CURRENT_USER, (LPCWSTR)path.u16string().c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        return false;
    }
    FunctionExitHelper_t exitHelper(
        [&]() {
            RegCloseKey(hKey);
        }
    );
    RegSetValueExW(hKey, L"AutoLoginUser", NULL, REG_SZ, (BYTE*)nameu16.c_str(), (nameu16.length() + 1) * sizeof(char16_t));
    DWORD value = 1;
    RegSetValueExW(hKey, L"RememberPassword", NULL, REG_DWORD, (BYTE*)&value, sizeof(value));
    return true;
}

bool SteamBrowserProtocolRun(uint32_t appid, int argc, const char* const* argv)
{
    std::wstring command(L"steam://run/");
    command += std::to_wstring(appid);
    SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };
    sei.lpFile = command.c_str();
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    auto res = ShellExecuteExW(&sei);
    return res;
}

bool SteamBrowserProtocolInstall(uint32_t appid) {
    std::wstring command(L"steam://install/");
    command += std::to_wstring(appid);
    SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW) };
    sei.lpFile = command.c_str();
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    auto res = ShellExecuteExW(&sei);
    return res;
}