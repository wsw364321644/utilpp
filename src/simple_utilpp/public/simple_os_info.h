#pragma once
#include "simple_export_ppdefs.h"
#include <cstdint>
#include <string>
SIMPLE_UTIL_EXPORT bool GetOSEnvironmentVariable(std::u8string_view VarName, char* outBuf, uint32_t* bufLen);
SIMPLE_UTIL_EXPORT bool SetOSEnvironmentVariable(std::u8string_view VarName, std::u8string_view buf);