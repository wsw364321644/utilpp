#include "simple_os_info.h"
#include "simple_os_defs.h"
#include "string_convert.h"
#include "FunctionExitHelper.h"
#include "char_buffer_extension.h"
bool GetOSEnvironmentVariable(std::u8string_view VarName, char* outBuf, uint32_t* bufLen)
{
    if (!bufLen) {
        return false;
    }
    auto VarName16=U8ToU16(VarName);
    auto wbuf = new wchar_t[0x7FFF];
    FunctionExitHelper_t wbufGuard([&]() {
        delete[] wbuf;
        });
    auto wlen= GetEnvironmentVariableW(ConvertU16ViewToWView(VarName16).data(), wbuf, 0x7FFF);
    if (wlen == 0) {
        return false;
    }
    *bufLen=U16ToU8Buf(wbuf, size_t(wlen), outBuf, *bufLen);
    return true;
}

bool SetOSEnvironmentVariable(std::u8string_view VarName, std::u8string_view buf)
{
    auto VarName16 = U8ToU16(VarName);
    auto buf16 = U8ToU16(buf);
    return SetEnvironmentVariableW(ConvertU16ViewToWView(VarName16).data(), ConvertU16ViewToWView(buf16).data());
}
