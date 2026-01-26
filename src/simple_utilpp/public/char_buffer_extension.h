#pragma once
#include <filesystem>
#include "RawFile.h"
#include "CharBuffer.h"
#include "simple_export_ppdefs.h"

inline const char* GetStringViewCStr(std::string_view view, FCharBuffer& buf) {
    if (view.data()[view.size()] == 0) {
        return view.data();
    }
    else {
        buf.Assign(view.data(), view.size());
        return buf.CStr();
    }
}

inline const wchar_t* GetWStringViewCStr(std::wstring_view view, FCharBuffer& buf) {
    if (view.data()[view.size()] == L'\0') {
        return view.data();
    }
    else {
        buf.Assign((const char*)view.data(), view.size()*sizeof(wchar_t));
        ((wchar_t*)buf.Data())[view.size()] = L'\0';
        return (const wchar_t*)buf.Data();
    }
}

SIMPLE_UTIL_EXPORT bool LoadFileToCharBuffer(FRawFile& file, FCharBuffer& buf, size_t extraSpace = 0);
