#pragma once  
#include "handle.h"
#include <functional>
#include <memory>
#include <ctime>
#include <string>
#include "simple_export_ppdefs.h"
typedef  std::function<void(std::string, std::time_t)>   CrashCallback;
SIMPLE_UTIL_EXPORT CommonHandle_t SetCrashHandle(CrashCallback);
SIMPLE_UTIL_EXPORT void ClearCrashHandle(CommonHandle_t);