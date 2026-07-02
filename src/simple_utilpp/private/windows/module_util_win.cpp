#include "module_util.h"
#include "string_convert.h"
#include "char_buffer_extension.h"
#include "PathBuf.h"
#include <stdlib.h>
#include <simple_os_defs.h>

#define threadtimeout 1000
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
static thread_local FPathBuf PathCache;
bool util_dll_path(char* path, size_t* size)
{
    if (!size) {
        return false;
    }
    PathCache.UpdatePathLenW(GetModuleFileNameW((HINSTANCE)&__ImageBase, PathCache.GetBufInternalW(), PATH_MAX));
    if (path) {
        *size = U16ToU8Buf((const char16_t*)PathCache.GetBufW(), PathCache.GetPathLenW(), path, *size);
    }
    else {
        *size = U16ToU8Buf((const char16_t*)PathCache.GetBufW(), PathCache.GetPathLenW(), nullptr, 0);
        return false;
    }
    return true;
}

bool util_dll_wpath(wchar_t* path, size_t* size)
{
    if (!size) {
        return false;
    }
    if (path) {
        *size = GetModuleFileNameW((HINSTANCE)&__ImageBase, path, *size);
        auto err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER) {
            *size = GetModuleFileNameW((HINSTANCE)&__ImageBase, PathCache.GetBufInternalW(), PATH_MAX);
            return false;
        }
    }
    else {
        *size = GetModuleFileNameW((HINSTANCE)&__ImageBase, PathCache.GetBufInternalW(), PATH_MAX);
    }
    return true;
}

bool util_exe_path(char* path, size_t* size)
{
    if (!size) {
        return false;
    }
    PathCache.UpdatePathLenW(GetModuleFileNameW(NULL, PathCache.GetBufInternalW(), PATH_MAX));
    if (path) {
        *size = U16ToU8Buf((const char16_t*)PathCache.GetBufW(), PathCache.GetPathLenW(), path, *size);
    }
    else {
        *size = U16ToU8Buf((const char16_t*)PathCache.GetBufW(), PathCache.GetPathLenW(), nullptr, 0);
        return false;
    }
    return true;
}

bool util_exe_wpath(wchar_t* path, size_t* size)
{
    if (!size) {
        return false;
    }
    if (path) {
        *size = GetModuleFileNameW(NULL, path, *size);
        auto err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER) {
            *size = GetModuleFileNameW(NULL, PathCache.GetBufInternalW(), PATH_MAX);
            return false;
        }
    }
    else {
        *size = GetModuleFileNameW(NULL, PathCache.GetBufInternalW(), PATH_MAX);
    }
    return true;
}

typedef struct DirInfoWin_t {
    DLL_DIRECTORY_COOKIE  dir_cookie;
}DirInfoWin_t;

void* add_module_dir(const char* path, size_t size)
{
    if (!path) {
        return nullptr;
    }
    PathCache.SetPath(std::string_view(path, size));
    auto wpath = PathCache.GetPrependFileNamespacesW();
    auto cookie = AddDllDirectory(wpath);
    return new DirInfoWin_t(cookie);


}

void* add_module_wdir(const wchar_t* path, size_t size)
{
    if (!path) {
        return nullptr;
    }
    FCharBuffer buf;
    path = GetWStringViewCStr(std::wstring_view(path, size), buf);
    auto cookie = AddDllDirectory(path);
    return new DirInfoWin_t(cookie);
}

void remove_module_dir(void* handle)
{
    if (!handle) {
        return;
    }
    auto pInfo=static_cast<DirInfoWin_t*>(handle);
    if (pInfo->dir_cookie) {
        RemoveDllDirectory(pInfo->dir_cookie);
        pInfo->dir_cookie = NULL;
    }
    delete pInfo;
    return;
}


struct sk_thread
{
    HANDLE thread;
    util_thread_func func;
    void* arg;
};

static DWORD WINAPI win_thread_func(void* arg);

util_thread_t util_create_thread(void* arg, util_thread_func func)
{
    struct sk_thread* p = (struct sk_thread*)calloc(1, sizeof(struct sk_thread));
    if (!p) {
        return NULL;
    }
    p->arg = arg;
    p->func = func;

    p->thread = CreateThread(NULL, 0, win_thread_func, p, 0, NULL);
    if (p->thread != INVALID_HANDLE_VALUE) {
        return p;
    }
    else {
        free(p);
        return NULL;
    }
}

void util_thread_join(util_thread_t  thread)
{
    struct sk_thread* p = (struct sk_thread*)thread;

    WaitForSingleObject(p->thread, threadtimeout);

    free(p);
}

DWORD WINAPI  win_thread_func(void* arg)
{
    struct sk_thread* p = (struct sk_thread*)arg;

    p->func(p->arg);

    CloseHandle(p->thread);
    return 0;
}
