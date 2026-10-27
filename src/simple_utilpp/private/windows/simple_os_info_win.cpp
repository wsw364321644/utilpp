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
    auto VarName16 = U8ToU16(VarName);
    auto wbuf = new wchar_t[0x7FFF];
    FunctionExitHelper_t wbufGuard([&]() {
        delete[] wbuf;
        });
    auto wlen = GetEnvironmentVariableW(ConvertU16ViewToWView(VarName16).data(), wbuf, 0x7FFF);
    if (wlen == 0) {
        return false;
    }
    *bufLen = U16ToU8Buf(wbuf, size_t(wlen), outBuf, *bufLen);
    return true;
}

bool GetOSEnvironmentVariable(std::u8string_view VarName, FCharBuffer& buf)
{
    auto VarName16 = U8ToU16(VarName);
    auto wbuf = new wchar_t[0x7FFF];
    FunctionExitHelper_t wbufGuard([&]() {
        delete[] wbuf;
        });
    auto wlen = GetEnvironmentVariableW(ConvertU16ViewToWView(VarName16).data(), wbuf, 0x7FFF);
    if (wlen == 0) {
        return false;
    }
     size_t bufLen=U16ToU8Buf(wbuf, size_t(wlen), nullptr, 0);
     buf.Reserve(bufLen + 1);
     bufLen=U16ToU8Buf(wbuf, size_t(wlen), buf.Data(), buf.Capacity());
     buf.SetLength(bufLen);
    return true;
}

bool SetOSEnvironmentVariable(std::u8string_view VarName, std::u8string_view buf)
{
    auto VarName16 = U8ToU16(VarName);
    auto buf16 = U8ToU16(buf);
    return SetEnvironmentVariableW(ConvertU16ViewToWView(VarName16).data(), ConvertU16ViewToWView(buf16).data());
}

bool GetOSEnvironmentStringsW(FCharBuffer& buf)
{
    auto block = GetEnvironmentStringsW();
    if (!block) return false;

    const wchar_t* p = block;
    while (*p || *(p + 1)) ++p;
    size_t words = static_cast<size_t>(p - block) + 2;
    buf.Reserve(words * sizeof(wchar_t));
    memcpy(buf.Data(), block, words * sizeof(wchar_t));
    buf.SetLength(words * sizeof(wchar_t));
    FreeEnvironmentStringsW(block);
    return true;
}

bool GetOSEnvironmentStringsA(FCharBuffer& buf)
{
    auto block = GetEnvironmentStringsA();
    if (!block) return false;

    const char* p = block;
    while (*p || *(p + 1)) ++p;
    size_t size = static_cast<size_t>(p - block) + 2;
    buf.Reserve(size);
    memcpy(buf.Data(), block, size);
    buf.SetLength(size);
    FreeEnvironmentStringsA(block);
    return true;
}
