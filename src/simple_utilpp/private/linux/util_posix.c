/**
 * util_posix.c
 * platform dependent function
 */

#include "module_util.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

struct sk_thread
{
    pthread_t thread;
    util_thread_func func;
    void* arg;
};

static void* posix_thread_func(void* arg);

util_thread_t util_create_thread(void* arg, util_thread_func func)
{
    struct sk_thread* p = calloc(1, sizeof(struct sk_thread));
    p->arg = arg;
    p->func = func;

    if (0 == pthread_create(&p->thread, NULL, posix_thread_func, p)) {
        return p;
    } else {
        free(p);
        return NULL;
    }
}

void util_thread_join(util_thread_t  thread)
{
    struct sk_thread* p = (struct sk_thread*)thread;
    void* ret = NULL;

    pthread_join(p->thread, &ret);

    free(p);
}

void* posix_thread_func(void* arg)
{
    struct sk_thread* p = (struct sk_thread*) arg;

    p->func(p->arg);

    return NULL;
}

extern char* g_dll_path;
int util_dll_path(char* path, size_t* size)
{
    *size = strlen(g_dll_path);
    memcpy(path, g_dll_path, *size+1);
    return 0;
}

#ifdef __linux__
#  include <unistd.h>
#  include <limits.h>
#elif __APPLE__
#  include <mach-o/dyld.h>
#endif

int util_exe_path(char* path, size_t* size)
{
#ifdef __APPLE__
    uint32_t len;
    if (_NSGetExecutablePath(path, &len) != 0) {
        path[0] = '\0'; // buffer too small (!)
        return -1;
    } else {
        // resolve symlinks, ., .. if possible
        char * abs_path = realpath(path, NULL);
        if (abs_path != NULL) {
            strcpy(path, abs_path);
            *size = strlen(path);
            free(abs_path);
            return 0;
        }
        return -1;
    }
#elif __linux__
    ssize_t len = readlink("/proc/self/exe", path, *size);
    if (len == -1) {
        return -1;
    }
    path[len] = '\0';
    *size = len;
    return 0;
#else
#error "Unknown platform"
#endif
    return 0;
}

