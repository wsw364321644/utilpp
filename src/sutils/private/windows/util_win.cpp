/**
 *   util_win.c
 */

#include "module_util.h"
#include "string_convert.h"
#include <stdlib.h>
#include <Windows.h>

#define threadtimeout 1000
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
int util_dll_path(char* path, size_t* size)
{
    wchar_t modulestr[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, modulestr, _countof(modulestr));
    auto u8str=U16ToU8((char16_t*)modulestr);
    memcpy(path, u8str.c_str(), u8str.size()+1);
    return 0;
}

int util_dll_wpath(wchar_t* path, size_t* size)
{
	wchar_t module[1024] = { 0 };
	GetModuleFileNameW((HINSTANCE)&__ImageBase, module, _countof(module));
	*size = wcslen(module);
	wcscpy(path, module);
	//memcpy(path, module, *size + 1);
	return 0;
}

int util_exe_path(char* path, size_t* size)
{
	
    wchar_t modulestr[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, modulestr, _countof(modulestr));
    auto u8str = U16ToU8((char16_t*)modulestr);
    memcpy(path, u8str.c_str(), u8str.size() + 1);
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
