#pragma once
#include <simple_os_defs.h>
#include "windows_util_export_defs.h"

constexpr char REG_COMMAND_PATH[] = "Shell\\Open\\Command";
constexpr char URL_PROTOCOL_KEY_NAME[] = "URL Protocol";
constexpr wchar_t URL_PROTOCOL_KEY_NAME_U16[] = L"URL Protocol";


WINDOWS_UTIL_API bool is_app_container(HANDLE process);
WINDOWS_UTIL_API bool is_uwp_window(HWND hwnd);
WINDOWS_UTIL_API BOOL is_main_window(HWND handle);
WINDOWS_UTIL_API bool is_64bit_process(HANDLE process);

WINDOWS_UTIL_API HWND get_uwp_actual_window(HWND parent);
WINDOWS_UTIL_API BOOL get_app_sid(HANDLE process, LPSTR* out);

WINDOWS_UTIL_API bool get_window_exe(LPSTR* const name, HWND window, fnmalloc mallocptr);
WINDOWS_UTIL_API void get_window_title(LPSTR* const name, HWND hwnd, fnmalloc mallocptr);
WINDOWS_UTIL_API void get_window_class(LPSTR* const name, HWND hwnd, fnmalloc mallocptr);
WINDOWS_UTIL_API HWND find_main_window(DWORD process_id);
WINDOWS_UTIL_API HWND find_window_by_title(const char* name);
WINDOWS_UTIL_API HMODULE get_process_module(DWORD process_id, LPSTR  file_name);
WINDOWS_UTIL_API HMODULE get_process_module_from_hanlde(HANDLE  hProcess, LPSTR  file_name);
WINDOWS_UTIL_API DWORD  get_process_file_name(DWORD process_id, HMODULE hMod, LPSTR  file_path, DWORD  nSize);
WINDOWS_UTIL_API DWORD  get_process_file_name_from_handle(HANDLE  hProcess, HMODULE hMod, LPSTR  file_path, DWORD  nSize);
WINDOWS_UTIL_API DWORD  get_process_file_base_name(DWORD process_id, HMODULE hMod, LPSTR  file_name, DWORD  nSize);
WINDOWS_UTIL_API DWORD  get_process_file_base_name_from_handle(HANDLE  hProcess, HMODULE hMod, LPSTR  file_name, DWORD  nSize);
WINDOWS_UTIL_API DWORD get_process_id_from_handle(HANDLE hProcess);

//not support  parallel
WINDOWS_UTIL_API HMODULE get_system_module(const char* module_name);
WINDOWS_UTIL_API HMODULE load_system_library(const char* module_name);

