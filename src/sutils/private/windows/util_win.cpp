/**
 *   util_win.c
 */

#include "module_util.h"
#include "string_convert.h"
#include <stdlib.h>
#include <Windows.h>
#include <TlHelp32.h>
#define threadtimeout 1000
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
int util_dll_path(char* path, size_t* size)
{
    wchar_t modulestr[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, modulestr, _countof(modulestr));
    auto u8str=U16ToU8((char16_t*)modulestr);
    
    memcpy(path, u8str.c_str(), u8str.size()+1);
    *size = u8str.size();
    return 0;
}

int util_dll_wpath(wchar_t* path, size_t* size)
{
    size_t buflen{ 128 };
    wchar_t* temppath = NULL;
    char* retval = NULL;
    size_t len = 0;
    if (path != nullptr) {
        buflen = *size;
        temppath = path;
    }
    else {
        temppath = (wchar_t*)malloc(buflen * sizeof(WCHAR));
    }
    while (true) {

        len = GetModuleFileNameW((HINSTANCE)&__ImageBase, temppath, buflen);
        /* if it truncated, then len >= buflen - 1 */
        /* if there was enough room (or failure), len < buflen - 1 */
        if (len < buflen - 1) {
            *size = len;
            break;
        }
        if (path) {
            break;
        }
        free(temppath);
        buflen *= 2;
        temppath = (wchar_t*)malloc(buflen * sizeof(WCHAR));
    }
    return 0;
}

uint64_t GetProcessParentId(uint64_t* id)
{
    int parent_pid = -1;
    HANDLE handle;
    PROCESSENTRY32 pe;
    DWORD current_pid = GetCurrentProcessId();

    if (id == nullptr) {
        current_pid = GetCurrentProcessId();
    }
    else {
        current_pid = *id;
    }

    pe.dwSize = sizeof(PROCESSENTRY32);
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(handle, &pe)) {
        do {
            if (pe.th32ProcessID == current_pid) {
                parent_pid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(handle, &pe));
    }

    CloseHandle(handle);
    return parent_pid;
}

int util_exe_path(char* path, size_t* size)
{
	
    wchar_t modulestr[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, modulestr, _countof(modulestr));
    auto u8str = U16ToU8((char16_t*)modulestr);
    memcpy(path, u8str.c_str(), u8str.size() + 1);
    *size = u8str.size();
    return 0;
}

int util_exe_wpath(wchar_t* path, size_t* size)
{
    size_t buflen{ 128 };
    wchar_t* temppath = NULL;
    char* retval = NULL;
    size_t len = 0;
    if (path != nullptr) {
        buflen = *size;
        temppath = path;
    }
    else {
        temppath =(wchar_t*) malloc(buflen * sizeof(WCHAR));
    }
    while (true) {

        len = GetModuleFileNameW(NULL, temppath, buflen);
        /* if it truncated, then len >= buflen - 1 */
        /* if there was enough room (or failure), len < buflen - 1 */
        if (len < buflen - 1) {
            *size = len;
            break;
        }
        if (path) {
            break;
        }
        free(temppath);
        buflen *= 2;
        temppath = (wchar_t*)malloc(buflen * sizeof(WCHAR));
    }
    return 0;
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
    struct sk_thread* p =(struct sk_thread*) calloc(1, sizeof(struct sk_thread));
    p->arg = arg;
    p->func = func;

    p->thread = CreateThread(NULL, 0, win_thread_func, p, 0, NULL);
    if (p->thread != INVALID_HANDLE_VALUE) {
        return p;
    } else {
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
    struct sk_thread* p = (struct sk_thread*) arg;

    p->func(p->arg);

    CloseHandle(p->thread);
    return 0;
}


bool GetProcessIdFromHandle(void* handle, uint64_t* id) {
    *id = GetProcessId(handle);
    return *id!=0;
}

bool GetProcessIdFromPipe(void* handle, uint64_t* id)
{
    ULONG out;
    if (GetNamedPipeClientProcessId(handle, &out)) {
        *id = out;
        return true;
    }
    return false;
}
