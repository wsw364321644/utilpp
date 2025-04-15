#pragma once
#include "windows_util_export_defs.h"
#include <simple_os_defs.h>
#include <winnt.h>
#include <winternl.h>
#include <iostream>

inline int AlignMent(DWORD size, DWORD align, DWORD addr = 0) {
    if (!(size % align)) return addr + size;
    return addr + (size / align + 1) * align;
}

//***********************
//PE信息获取函数簇
//time:2020/11/2
//***********************
WINDOWS_UTIL_API PIMAGE_DOS_HEADER GetDosHeader(_In_ const char* pBase);
WINDOWS_UTIL_API PIMAGE_NT_HEADERS GetNtHeader(_In_ const char* pBase);
WINDOWS_UTIL_API PIMAGE_FILE_HEADER GetFileHeader(_In_ const char* pBase);
WINDOWS_UTIL_API PIMAGE_OPTIONAL_HEADER GetOptHeader(_In_ const char* pBase);
WINDOWS_UTIL_API PIMAGE_SECTION_HEADER GetLastSec(_In_ const char* pBase);
WINDOWS_UTIL_API PIMAGE_SECTION_HEADER GetSecByName(_In_ const char* pBase, _In_ const char* name);
WINDOWS_UTIL_API PIMAGE_SECTION_HEADER FindSecByVirtualAddress(const char* pBase, DWORD  address);





//*********************
//增添区段
//time:2020/11/6
//*********************
WINDOWS_UTIL_API char* AddSec(_In_ char*& hpe, _In_ DWORD& filesize, _In_ const char* secname, _In_ const int secsize);

WINDOWS_UTIL_API void FixStub(char* targetDllbase, char* stubDllbase, DWORD targetNewScnRva, DWORD stubTextRva);
WINDOWS_UTIL_API void ModResourceDirectory( char* srcImage, IMAGE_SECTION_HEADER& desSection);

WINDOWS_UTIL_API PPEB get_peb();

WINDOWS_UTIL_API void EnumExportedFunctions(const char*, void (*callback)(const char*));

typedef bool (*EnumExportedCallback)(const char*, void*);
WINDOWS_UTIL_API void EnumExportedFunctionsHandle(HMODULE, EnumExportedCallback);