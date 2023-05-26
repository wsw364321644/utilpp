#pragma once
#include <Windows.h>
#include <winnt.h>
#include <winternl.h>



//***********************
//PE信息获取函数簇
//time:2020/11/2
//***********************
PIMAGE_DOS_HEADER GetDosHeader(_In_ const char* pBase);
PIMAGE_NT_HEADERS GetNtHeader(_In_ const char* pBase);
PIMAGE_FILE_HEADER GetFileHeader(_In_ const char* pBase);
PIMAGE_OPTIONAL_HEADER GetOptHeader(_In_ const char* pBase);
PIMAGE_SECTION_HEADER GetLastSec(_In_ const char* pBase);
PIMAGE_SECTION_HEADER GetSecByName(_In_ const char* pBase, _In_ const char* name);





//*********************
//增添区段
//time:2020/11/6
//*********************
char* AddSec(_In_ char*& hpe, _In_ DWORD& filesize, _In_ const char* secname, _In_ const int secsize);

void FixStub(DWORD targetDllbase, DWORD stubDllbase, DWORD targetNewScnRva, DWORD stubTextRva);

PPEB get_peb();

void EnumExportedFunctions(const char*, void (*callback)(const char*));

typedef bool (*EnumExportedCallback)(const char*, void*);
void EnumExportedFunctionsHandle(HMODULE, EnumExportedCallback);