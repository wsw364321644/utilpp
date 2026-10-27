
#include "simple_os_info.h"
#include "simple_os_defs.h"
#include "string_convert.h"
bool GetOSEnvironmentVariable(std::u8string_view VarName, char* outBuf, uint32_t* bufLen)
{
    if (!bufLen) {
        return false;
    }
    char* ptr = std::getenv((const char *)VarName.data());
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
    auto pair=std::format("{}={}", ConvertU8ViewToView(VarName), ConvertU8ViewToView(buf));
    if (putenv(pair.c_str())!=0){
        return false;
    }
    return true;
}

bool GetOSEnvironmentStringsW(FCharBuffer& buf)
{
    for (char** env = environ; env && *env; ++env)
    {
        const char* entry = *env;
        auto str16=U8ToU16(entry,strlen(entry));
        buf.Append((const char *)str16.data(), str16.length()*sizeof(wchar_t));
        buf.Append((const char *)L'\0', sizeof(wchar_t));
    }
    buf.Append((const char *)L'\0', sizeof(wchar_t));
    return true;
}

bool GetOSEnvironmentStringsA(FCharBuffer& buf)
{
    for (char** env = environ; env && *env; ++env)
    {
        const char* entry = *env;
        size_t len = std::strlen(entry);
        buf.Append(entry, len);
        buf.Put('\0');
    }
    buf.Put('\0');
    return true;
}