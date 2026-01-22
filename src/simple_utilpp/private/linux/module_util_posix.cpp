/**
 * util_posix.c
 * platform dependent function
 */

#include "module_util.h"
#include "char_buffer_extension.h"
#include "FunctionExitHelper.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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
    struct sk_thread* p = (struct sk_thread*)calloc(1, sizeof(struct sk_thread));
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

bool util_dll_path(char* path, size_t* size)
{
    Dl_info info;
    if (dladdr((void*)dummy_so_function, &info)==0) {
        return false;
    }
    auto rsize=strlen(info.dli_fname);
    FunctionExitHelper_t sizeGuard(
        [&]() { 
            *size = strlen(info.dli_fname); 
        }
    );
    if(*size<rsize) {
        return false;
    }
    memcpy(path, info.dli_fname, rsize);
    return true;
}
bool util_dll_wpath(wchar_t* path, size_t* size)
{
    size_t len = *size;
    auto bres=util_dll_path((char*)path, size*sizeof(wchar_t));
    if (!bres) {
        *size=*size*2;
        return false;
    }
    auto u16str=U8ToU16(std::string_view((const char*)path, *size));
    FunctionExitHelper_t sizeGuard(
        [&]() { 
            *size = u16str.length(); 
        }
    );
    if(u16str.length()>len) {
        return false;
    }
    memcpy(path, u16str.c_str(), u16str.length()*sizeof(wchar_t));
    *size = u16str.length();
    return true;
}

#ifdef __linux__
#  include <unistd.h>
#  include <limits.h>
#elif __APPLE__
#  include <mach-o/dyld.h>
#endif

bool util_exe_path(char* path, size_t* size)
{
#ifdef __APPLE__
    uint32_t len;
    if (_NSGetExecutablePath(path, &len) != 0) {
        path[0] = '\0'; // buffer too small (!)
        return false;
    } else {
        // resolve symlinks, ., .. if possible
        char * abs_path = realpath(path, NULL);
        if (abs_path != NULL) {
            strcpy(path, abs_path);
            *size = strlen(path);
            free(abs_path);
            return true;
        }
        return false;
    }
#elif __linux__
    ssize_t len = readlink("/proc/self/exe", path, *size);
    if (len == -1) {
        return false;
    }
    path[len] = '\0';
    *size = len;
    return true;
#else
#error "Unknown platform"
#endif
    return false;
}
bool util_exe_wpath(wchar_t* path, size_t* size)
{
    size_t len = *size;
    auto bres=util_exe_path((char*)path, size*sizeof(wchar_t));
    *size=*size*2;
    if (!bres) {
        return false;
    }
    auto u16str=U8ToU16(std::string_view((const char*)path, *size));
    FunctionExitHelper_t sizeGuard(
        [&]() { 
            *size = u16str.length(); 
        }
    );
    if(u16str.length()>len) {
        return false;
    }
    memcpy(path, u16str.c_str(), u16str.length()*sizeof(wchar_t));
    *size = u16str.length();
    return true;
}

bool add_module_path(const char* path, size_t size)
{
    if (!path) {
        return false;
    }
    const char* current_path = getenv("LD_LIBRARY_PATH");
    FCharBuffer buf;
    buf.Reverse(strlen(current_path)+size+2);
    buf.Assign(path, size);
    if (current_path) {
        buf.Append(":");
        buf.Append(current_path);
    }
    return setenv("LD_LIBRARY_PATH", buf.CStr(), 1)==0;
}

bool add_module_wpath(const wchar_t* path, size_t size)
{
    if (!path) {
        return false;
    }
    auto u8str=U16ToU8(std::u16string_view((const char16_t*)path, size));
    return add_module_path(u8str.c_str(), u8str.length());
}
