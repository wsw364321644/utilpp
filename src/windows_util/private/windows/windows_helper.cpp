#include "windows_helper.h"
#include <simple_os_defs.h>
#include <psapi.h>
#include <sddl.h>
#include <string_convert.h>
#include <string_buffer.h>

static bool bsystem_path_inited;
static WCHAR system_path[MAX_PATH];

bool is_app_container(HANDLE process)
{
    DWORD size_ret;
    DWORD ret = 0;
    HANDLE token;

    if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
        BOOL success = GetTokenInformation(token, TokenIsAppContainer,
            &ret, sizeof(ret),
            &size_ret);
        if (!success) {
            const DWORD error = GetLastError();
            return false;
        }

        CloseHandle(token);
    }
    return !!ret;
}

bool is_uwp_window(HWND hwnd)
{
    wchar_t name[256];

    name[0] = 0;
    if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
        return false;

    return wcscmp(name, L"ApplicationFrameWindow") == 0;
}

bool is_64bit_process(HANDLE process)
{
    BOOL x86 = true;
    bool success;
#ifndef _WIN64
    success = !!IsWow64Process(GetCurrentProcess(), &x86);
    if (!success) {
        return false;
    }
    if (!x86) {
        return false;
    }
#endif
    success = !!IsWow64Process(process, &x86);
    if (!success) {
        return false;
    }
    return !x86;


}

BOOL is_main_window(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

HWND get_uwp_actual_window(HWND parent)
{
    DWORD parent_id = 0;
    HWND child;

    GetWindowThreadProcessId(parent, &parent_id);
    child = FindWindowEx(parent, NULL, NULL, NULL);

    while (child) {
        DWORD child_id = 0;
        GetWindowThreadProcessId(child, &child_id);

        if (child_id != parent_id)
            return child;

        child = FindWindowEx(parent, child, NULL, NULL);
    }

    return NULL;
}

BOOL get_app_sid(HANDLE process, LPSTR* out)
{
    DWORD size_ret;
    BOOL success;
    HANDLE token;

    if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
        DWORD info_len = GetSidLengthRequired(12) +
            sizeof(TOKEN_APPCONTAINER_INFORMATION);

        PTOKEN_APPCONTAINER_INFORMATION info = (PTOKEN_APPCONTAINER_INFORMATION)malloc(info_len);

        success = GetTokenInformation(token, TokenAppContainerSid, info,
            info_len, &size_ret);
        if (success)
            //free memory by localfree
            ConvertSidToStringSidA(info->TokenAppContainer, out);

        free(info);
        CloseHandle(token);
        return true;
    }
    return false;
}

bool get_window_exe(LPSTR* const name, HWND window, fnmalloc mallocptr)
{
    wchar_t wname[MAX_PATH];

    bool success = false;
    HANDLE process = NULL;
    const char* slash;
    DWORD id;
    std::string u8str;
    GetWindowThreadProcessId(window, &id);
    if (id == GetCurrentProcessId())
        return false;

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
    if (!process)
        goto fail;

    if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
        goto fail;

    u8str = U16ToU8((char16_t*)wname,GetStringLengthW(wname));

    slash = strrchr(u8str.c_str(), '\\');
    if (!slash)
        goto fail;
    *name = (char*)mallocptr(strlen(slash + 1) + 1);
    strcpy(*name, slash + 1);
    success = true;

fail:
    if (!success) {
        *name = (char*)mallocptr(strlen("unknown") + 1);
        strcpy(*name, "unknown");
    }

    CloseHandle(process);
    return true;
}

void get_window_title(LPSTR* const name, HWND hwnd, fnmalloc mallocptr)
{
    int len;

    len = GetWindowTextLengthW(hwnd);
    if (!len)
        return;


    wchar_t* temp;

    temp = (wchar_t*)malloc(sizeof(wchar_t) * (len + 1));
    if (!temp)
        return;

    if (GetWindowTextW(hwnd, temp, len + 1)) {
        auto u8str = U16ToU8((char16_t*)temp,GetStringLengthW(temp));
        *name = (char*)mallocptr(u8str.size() + 1);
        lstrcpyA(*name, u8str.c_str());
    }

    free(temp);
}

void get_window_class(LPSTR* const name, HWND hwnd, fnmalloc mallocptr)
{
    wchar_t temp[256];

    temp[0] = 0;
    if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t))) {
        auto u8str = U16ToU8((char16_t*)temp, GetStringLengthW(temp));
        *name = (char*)mallocptr(u8str.size() + 1);
        lstrcpyA(*name, u8str.c_str());
    }
}
struct handle_data {
    unsigned long process_id;
    HWND window_handle;
};



BOOL CALLBACK find_main_window_callback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !is_main_window(handle))
        return TRUE;
    data.window_handle = handle;
    return FALSE;
}
HWND find_main_window(DWORD process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(find_main_window_callback, (LPARAM)&data);
    if (is_uwp_window(data.window_handle))
        data.window_handle = get_uwp_actual_window(data.window_handle);
    return data.window_handle;
}

typedef struct find_window_by_title_data_t {
    const char* name;
    HWND handle;
}find_window_by_title_data_t;
BOOL CALLBACK find_window_by_title_callback(HWND handle, LPARAM lParam)
{
    find_window_by_title_data_t& data = *(find_window_by_title_data_t*)lParam;
    char title[256];
    GetWindowTextA(handle, title, sizeof(title));
    if (strstr(title, data.name) != NULL) {
        data.handle = handle;
        return FALSE;
    }
    return TRUE;
}
HWND find_window_by_title(const char* name)
{
    find_window_by_title_data_t data;
    data.name = name;
    data.handle = 0;
    EnumWindows(find_window_by_title_callback, (LPARAM)&data);
    return data.handle;
}

WINDOWS_UTIL_API HMODULE get_process_module(DWORD process_id, LPSTR file_name)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, process_id);
    auto res = get_process_module_from_hanlde(hProcess, file_name);
    CloseHandle(hProcess);
    return res;
}

WINDOWS_UTIL_API HMODULE get_process_module_from_hanlde(HANDLE hProcess, LPSTR file_name)
{
    HMODULE* hModList{NULL};
    HMODULE retMod{ NULL };
    DWORD cbNeeded, cbNewNeeded;
    char moduleName[MAX_PATH] = "<unknown>";
    if (EnumProcessModules(hProcess, NULL, 0, &cbNeeded)==0) {
        return NULL;
    }
    hModList = new HMODULE[cbNeeded];
    EnumProcessModules(hProcess, hModList, cbNeeded, &cbNewNeeded);
    int i = 0;
    for (; i < cbNeeded; i++) {
        get_process_file_base_name_by_handle(hProcess, hModList[i], moduleName, MAX_PATH);
        auto res=strstr(moduleName, file_name);
        if (res != nullptr) {
            break;
        }
    }
    if (i < cbNeeded) {
        retMod = hModList[i];
    }
    delete[] hModList;
    return retMod;
}


DWORD  get_process_file_name(DWORD process_id, HMODULE hMod, LPSTR  file_path, DWORD  size) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, process_id);
    DWORD res=get_process_file_name_by_handle(hProcess, hMod,file_path, size);
    CloseHandle(hProcess);
    return res;
}

DWORD  get_process_file_name_by_handle(HANDLE  hProcess, HMODULE hMod, LPSTR  file_path, DWORD  size) {
    return GetModuleFileNameExA(hProcess, hMod, file_path, size);
    //return GetProcessImageFileNameA(hProcess, file_path, size);
}

WINDOWS_UTIL_API DWORD get_process_file_base_name(DWORD process_id, HMODULE hMod, LPSTR file_name, DWORD nSize)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, process_id);
    DWORD res = get_process_file_base_name_by_handle(hProcess, hMod, file_name, nSize);
    CloseHandle(hProcess);
    return res;
}

WINDOWS_UTIL_API DWORD get_process_file_base_name_by_handle(HANDLE hProcess, HMODULE hMod, LPSTR file_name, DWORD nSize)
{
    return  GetModuleBaseNameA(GetCurrentProcess(), hMod, file_name,nSize);
}



HANDLE create_mutex(const char* name, BOOL is_app)
{
    if (is_app) {

    }
    return CreateMutexA(NULL, false, name);
}

HANDLE open_mutex(const char* name, BOOL is_app)
{
    if (is_app) {

    }
    return OpenMutexA((SYNCHRONIZE), false, name);
}

HANDLE create_event(const char* name, BOOL is_app)
{
    if (is_app) {

    }
    return CreateEventA(NULL, false, false, name);
}

HANDLE open_event(const char* name, BOOL is_app)
{
    if (is_app) {

    }
    return OpenEventA((EVENT_MODIFY_STATE | SYNCHRONIZE), false, name);
}

UINT get_system_module_path(WCHAR* base_path, const char* module_name) {
    if (!bsystem_path_inited) {
        UINT ret = GetSystemDirectoryW(system_path, MAX_PATH);
        if (!ret) {
            // log("Failed to get windows system path: %lu", GetLastError());
            return ret;
        }
        bsystem_path_inited = true;
    }
    lstrcpyW(base_path, system_path);
    lstrcatW(base_path, L"\\");
    auto module_name16 = U8ToU16(module_name, GetStringLength(module_name));
    lstrcatW(base_path, (WCHAR*)module_name16.c_str());
    return NULL;
}
HMODULE get_system_module(const char* module_name)
{

    WCHAR base_path[MAX_PATH];
    if (get_system_module_path(base_path, module_name) != NULL) {
        return NULL;
    }
    return GetModuleHandleW(base_path);
}

HMODULE load_system_library(const char* module_name)
{
    WCHAR base_path[MAX_PATH];
    if (get_system_module_path(base_path, module_name) != NULL) {
        return NULL;
    }
    return LoadLibraryW(base_path);
}
