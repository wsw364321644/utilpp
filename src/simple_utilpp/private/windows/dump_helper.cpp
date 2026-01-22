
#include "dump_helper.h"
#include "string_convert.h"
#include "handle.h"
#include <simple_os_defs.h>
#include <DbgHelp.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <vector>

// #pragma comment(lib, "dbghelp.lib")

struct DumpHandle_t : CommonHandle32_t
{
    DumpHandle_t(const CommonHandle32_t handle) :CommonHandle32_t(handle) {}
    DumpHandle_t() : CommonHandle32_t() {}
    inline static std::atomic<CommonHandleID_t> DumpCount;
    static std::atomic_bool initialized;
    static std::vector<DumpHandle_t> DumpHandles;
    CrashCallback funcptr;
};
std::atomic_bool DumpHandle_t::initialized(false);
std::vector<DumpHandle_t> DumpHandle_t::DumpHandles;

BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
{
    if (pModuleName == 0)
    {
        return FALSE;
    }

    WCHAR szFileName[_MAX_FNAME] = L"";
    _wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

    if (_wcsicmp(szFileName, L"ntdll") == 0)
        return TRUE;

    return FALSE;
}

BOOL CALLBACK MiniDumpCallback(PVOID pParam,
    const PMINIDUMP_CALLBACK_INPUT pInput,
    PMINIDUMP_CALLBACK_OUTPUT pOutput)
{
    if (pInput == 0 || pOutput == 0)
        return FALSE;

    switch (pInput->CallbackType)
    {
    case ModuleCallback:
        if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
            if (!IsDataSectionNeeded(pInput->Module.FullPath))
                pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
    case IncludeModuleCallback:
    case IncludeThreadCallback:
    case ThreadCallback:
    case ThreadExCallback:
        return TRUE;
    default:;
    }

    return FALSE;
}

typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

void CreateMiniDump(PEXCEPTION_POINTERS pep, LPCTSTR strFileName)
{
    auto strFileNamew = U8ToU16(strFileName, GetStringLength(strFileName));
    HANDLE hFile = CreateFileW((LPCWSTR)strFileNamew.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
    {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = NULL;

        MINIDUMP_CALLBACK_INFORMATION mci;
        mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
        mci.CallbackParam = 0;

        HMODULE hDll = NULL;
        MINIDUMPWRITEDUMP pMiniDumpWriteDump = NULL;
        _MINIDUMP_EXCEPTION_INFORMATION ExceptionInformation = { 0 };

        // load MiniDumpWriteDump
        hDll = LoadLibraryW(L"DbgHelp.dll");
        if (hDll)
        {
            pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump");
            if (pMiniDumpWriteDump)
            {
                pMiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, (pep != 0) ? &mdei : 0, NULL, &mci);
            }
        }

        CloseHandle(hFile);
    }
}

LONG __stdcall MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
    const std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    tm utc_tm = *gmtime(&t_c);
    // curl_global_init(CURL_GLOBAL_ALL);
    char dumpfile[128];
    // sprintf(dumpfile, "user=%dtime=%4d-%02d-%02d--%02d-%02d-%02d.dmp", g_user, sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
    sprintf(dumpfile, "time=%4d-%02d-%02d--%02d-%02d-%02d.dmp", utc_tm.tm_year + 1900, utc_tm.tm_mon + 1, utc_tm.tm_mday, utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
    CreateMiniDump(pExceptionInfo, dumpfile);
    // UpLoadDump(dumpfile);
    // HttpUploaderrinfo(dumpfile, sys);
    //	MessageBox(0, "Error", "error", MB_OK);
    /*printf("Error   address   %x/n", pExceptionInfo->ExceptionRecord->ExceptionAddress);
    printf("CPU   register:/n");
    printf("eax   %x   ebx   %x   ecx   %x   edx   %x/n", pExceptionInfo->ContextRecord->Eax,
    pExceptionInfo->ContextRecord->Ebx, pExceptionInfo->ContextRecord->Ecx,
    pExceptionInfo->ContextRecord->Edx);*/

    for (auto handle : DumpHandle_t::DumpHandles)
    {
        handle.funcptr(dumpfile, t_c);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

CommonHandle32_t SetCrashHandle(CrashCallback infunc)
{
    DumpHandle_t::DumpHandles.emplace_back(DumpHandle_t::DumpCount);
    DumpHandle_t& handle = DumpHandle_t::DumpHandles.back();
    handle.funcptr = infunc;
    bool expected = false;
    if (DumpHandle_t::initialized.compare_exchange_strong(expected, true))
    {
        SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
    }
    return handle;
}

void ClearCrashHandle(CommonHandle32_t handle)
{
    auto result = std::find_if(std::begin(DumpHandle_t::DumpHandles), std::end(DumpHandle_t::DumpHandles), [&](const CommonHandle32_t& handleitr) -> bool
        { return handleitr == handle; });

    if (result != std::end(DumpHandle_t::DumpHandles))
    {
        DumpHandle_t::DumpHandles.erase(result);
    }
}
