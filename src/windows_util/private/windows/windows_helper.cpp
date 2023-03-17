#include "windows_helper.h"
#include <Windows.h>
#include <psapi.h>
#include <string_convert.h>
#include <string_buffer.h>
bool ms_is_uwp_window(HWND hwnd)
{
    wchar_t name[256];

    name[0] = 0;
    if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
        return false;

    return wcscmp(name, L"ApplicationFrameWindow") == 0;
}

HWND ms_get_uwp_actual_window(HWND parent)
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

bool ms_get_window_exe(char** const name, HWND window, fnmalloc mallocptr)
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

    u8str = U16ToU8((char16_t*)wname);

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

void ms_get_window_title(char** const name, HWND hwnd, fnmalloc mallocptr)
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
        auto u8str = U16ToU8((char16_t*)temp);
        *name = (char*)mallocptr(u8str.size() + 1);
        strcpy(*name, u8str.c_str());
    }

    free(temp);
}

void ms_get_window_class(char** const name, HWND hwnd, fnmalloc mallocptr)
{
    wchar_t temp[256];

    temp[0] = 0;
    if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t))) {
        auto u8str = U16ToU8((char16_t*)temp);
        *name = (char*)mallocptr(u8str.size() + 1);
        strcpy(*name, u8str.c_str());
    }
}
struct handle_data {
    unsigned long process_id;
    HWND window_handle;
};


BOOL is_main_window(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}
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
HWND find_main_window(unsigned long process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(find_main_window_callback, (LPARAM)&data);
    if (ms_is_uwp_window(data.window_handle))
        data.window_handle = ms_get_uwp_actual_window(data.window_handle);
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
