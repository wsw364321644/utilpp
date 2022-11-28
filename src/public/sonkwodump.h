#pragma once  
#include "handle.h"
#include <functional>
#include <memory>
#include <ctime>
#include <string>
namespace sonkwo {
    typedef  std::function<void(std::string, std::time_t)>   CrashCallback;
    CommonHandle_t SetCrashHandle(CrashCallback);
    void ClearCrashHandle(CommonHandle_t);
}
