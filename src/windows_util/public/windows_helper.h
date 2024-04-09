#include <simple_os_defs.h>


#define REG_COMMAND_PATH "shell\\open\\command";

#ifdef __cplusplus
extern "C" {
#endif
    bool is_app_container(HANDLE process);
    bool is_uwp_window(HWND hwnd);
    BOOL is_main_window(HWND handle);
    bool is_64bit_process(HANDLE process);

    HWND get_uwp_actual_window(HWND parent);
    BOOL get_app_sid(HANDLE process, LPSTR* out);

    bool get_window_exe(LPSTR* const name, HWND window, fnmalloc mallocptr);
    void get_window_title(LPSTR* const name, HWND hwnd, fnmalloc mallocptr);
    void get_window_class(LPSTR* const name, HWND hwnd, fnmalloc mallocptr);
    HWND find_main_window(DWORD process_id);
    HWND find_window_by_title(const char* name);
    DWORD  get_process_file_name(DWORD process_id, LPSTR  file_path, DWORD  nSize);
    DWORD  get_process_file_name_by_handle(HANDLE  hProcess, LPSTR  file_path, DWORD  nSize);

    HANDLE create_mutex(const char* name, BOOL is_app);
    HANDLE open_mutex(const char* name, BOOL is_app);
    HANDLE create_event(const char* name, BOOL is_app);
    HANDLE open_event(const char* name, BOOL is_app);

    //not support  parallel
    HMODULE get_system_module(const char* module_name);
    HMODULE load_system_library(const char* module_name);
#ifdef __cplusplus
}
#endif
