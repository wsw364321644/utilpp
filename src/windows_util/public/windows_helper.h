
#include <defs.h>


#define REG_COMMAND_PATH "shell\\open\\command";

#ifdef __cplusplus
extern "C" {
#endif
    bool ms_is_uwp_window(HWND hwnd);
    HWND ms_get_uwp_actual_window(HWND parent);
    bool ms_get_window_exe(char** const name, HWND window, fnmalloc mallocptr);
    void ms_get_window_title(char** const name, HWND hwnd, fnmalloc mallocptr);
    void ms_get_window_class(char** const name, HWND hwnd, fnmalloc mallocptr);
    HWND find_main_window(unsigned long process_id);
    HWND find_window_by_title(const char* name);

    BOOL is_main_window(HWND handle);

    bool is_64bit_process(HANDLE process);
#ifdef __cplusplus
}
#endif
