/**
 * util_posix.c
 * platform dependent function
 */

#include "module_util.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
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





bool isProcessUsingPipe(const char* pid, const char* pipePath) {
    std::string fdDir=std::string("/proc/")+pid+"/fd";
    DIR* dir = opendir(fdDir.c_str());
    if (!dir) return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_LNK) {
            char linkPath[PATH_MAX];
            snprintf(linkPath, sizeof(linkPath), "%s/%s", fdDir.c_str(), entry->d_name);

            char target[PATH_MAX];
            ssize_t len = readlink(linkPath, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                if (std::string(target) == pipePath) {
                    closedir(dir);
                    return true;
                }
            }
        }
    }
    closedir(dir);
    return false;
}


bool GetProcessIdFromPipe(void* handle, uint64_t* id)
{
    DIR* procDir = opendir("/proc");
    if (!procDir) {
        std::cerr << "Failed to open /proc directory." << std::endl;
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if the entry is a directory and represents a process ID
        if (entry->d_type == DT_DIR &&std::all_of(entry->d_name, entry->d_name + strlen(entry->d_name), ::isdigit)) {
            if (isProcessUsingPipe(entry->d_name, (const char*)handle)) {
                *id = std::stoi(entry->d_name);
                break;
            }
        }
    }

    closedir(procDir);
    return true;
}

