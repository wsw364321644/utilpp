/**
 *  dll_posix.c
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>

char * g_dll_path;

__attribute__((constructor))
void on_load(void) {
    Dl_info info;
    if (dladdr(on_load, &info) == 0) {
        // failed to get info
    } else {
        g_dll_path = strdup(info.dli_fname);
    }
}
