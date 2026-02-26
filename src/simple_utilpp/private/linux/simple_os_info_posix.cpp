
#include "simple_os_info.h"
#include "simple_os_defs.h"
#include "string_convert.h"
bool GetOSEnvironmentVariable(std::u8string_view VarName, char* outBuf, uint32_t* bufLen)
{
    if (!bufLen) {
        return false;
    }
    char* ptr = std::getenv();
    if (!ptr) {
        *bufLen=0;
        return false;
    }
    size_t len = std::strlen(ptr);
    if (*bufLen<=len) {
        *bufLen=len+1;
        return false;
    }
    std::memcpy(outBuf, ptr, len+1);
    *bufLen=len;
    return true;
}

bool SetOSEnvironmentVariable(std::u8string_view VarName, std::u8string_view buf)
{
    char* bufPtr = new char[VarName.length() + buf.length() + 2]; 
    FunctionExitHelper_t bufGuard([&]() { delete[] bufPtr; });
    memcpy(bufPtr, VarName.data(), VarName.length());
    bufPtr[VarName.length()] = '=';
    memcpy(bufPtr + VarName.length() + 1, buf.data(), buf.length());
    bufPtr[VarName.length() + 1 + buf.length()] = '\0';
    if (putenv(bufPtr)!=0){
        return false;
    }
    return true;
}
