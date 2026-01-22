#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cuchar>
#ifdef WIN32
#include <simple_os_defs.h>
#include <Strsafe.h>
#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <cstring>
#endif
inline size_t GetStringLength(const char* u8Str)
{
    size_t inStrLen;
#ifdef WIN32
    const size_t inStrMax = INT_MAX - 1;
    HRESULT hr = ::StringCchLengthA(u8Str, inStrMax, &inStrLen);
    if (FAILED(hr))
        return -1;
#else
    // std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
    // return cvt.from_bytes(u8Str);
    inStrLen = std::char_traits<char>::length(u8Str);
#endif
    return inStrLen;
}

inline size_t GetU8StringLength(const char8_t* u8Str)
{
    return GetStringLength((const char*)u8Str);
}

inline size_t GetStringLengthW(const wchar_t* Str)
{
    size_t inStrLen{ 0 };
#ifdef WIN32
    const size_t inStrMax = INT_MAX - 1;
    LPWSTR inStr = (LPWSTR)Str;
    HRESULT hr = ::StringCchLengthW(inStr, inStrMax, &inStrLen);
    if (FAILED(hr))
        return -1;
#else
    // std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
    // return cvt.to_bytes(u16Str);
    // setlocale(LC_ALL, "");
    // char* locstr = setlocale(LC_CTYPE, NULL);
    // char* encoding = nl_langinfo(CODESET);
    inStrLen = std::char_traits<wchar_t>::length(Str);
#endif
    return inStrLen;
}

inline size_t GetU16StringLength(const char16_t* Str)
{
    return GetStringLengthW((const wchar_t*)Str);
}

inline size_t U8ToU16Buf(const char8_t* u8Str, size_t inStrLen, wchar_t* outBuf, size_t bufLen) {
    if ((u8Str == NULL) || (*u8Str == '\0'))
        return 0;
#ifdef WIN32
    char* in = (char*)u8Str;
    int result = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        in,
        (int)inStrLen,
        (LPWSTR)outBuf,
        bufLen);
    return result;
#else
    char* out = (char*)outBuf;
    char* in = (char*)u8Str;
    size_t outbytes = bufLen*sizeof(wchar_t);
    iconv_t conv = iconv_open("UTF-16le", "UTF-8");
    if (conv == (iconv_t)-1)
    {
        return -1;
    }
    size_t res=iconv(conv, &in, &inStrLen, &out, &outbytes) ;
    iconv_close(conv);
    return res;
#endif
}
inline size_t U8ToU16Buf(const char* u8Str, size_t inStrLen, wchar_t* outBuf, size_t bufLen) {
    return U8ToU16Buf((const char8_t*)u8Str, inStrLen, outBuf, bufLen);
}
inline size_t U16ToU8Buf(const char16_t* u16Str, size_t inStrLen, char* outBuf, size_t bufLen) {
    if ((u16Str == NULL) || (*u16Str == L'\0'))
        return 0;
#ifdef WIN32
    LPWSTR inStr = (LPWSTR)u16Str;
#if (WINVER >= 0x0600)
    DWORD flags = WC_ERR_INVALID_CHARS;
#else
    DWORD flags = 0;
#endif
    int result = ::WideCharToMultiByte(CP_UTF8,
        flags,
        inStr,
        static_cast<int>(inStrLen),
        outBuf,
        bufLen,
        NULL,
        NULL);
    return result;
#else
    char* out = outBuf;
    char* in = (char*)u16Str;
    size_t outbytes = bufLen;
    iconv_t conv = iconv_open("UTF-8", "utf-16le");
    if (conv == (iconv_t)-1)
    {
        return -1;
    }
    size_t res=iconv(conv, &in, &inStrLen, &out, &outbytes) ;
    iconv_close(conv);
    return res;
#endif
}
inline size_t U16ToU8Buf(const wchar_t* u16Str, size_t inStrLen, char* outBuf, size_t bufLen) {
    return U16ToU8Buf((const char16_t*)u16Str, inStrLen, outBuf, bufLen);
}
inline std::string U16ToU8(const char16_t* u16Str, size_t inStrLen) {
#ifdef WIN32
    if ((u16Str == NULL) || (*u16Str == L'\0'))
        return "";
    LPWSTR inStr = (LPWSTR)u16Str;
#if (WINVER >= 0x0600)
    DWORD flags = WC_ERR_INVALID_CHARS;
#else
    DWORD flags = 0;
#endif

    int outStrLen = ::WideCharToMultiByte(CP_UTF8,
        flags,
        inStr,
        (int)inStrLen,
        NULL,
        0,
        NULL,
        NULL);
    if (outStrLen == 0)
        return "";

    std::vector<char> outBuf;
    outBuf.assign(outStrLen+1, '\0');
    auto result = ::WideCharToMultiByte(CP_UTF8,
        flags,
        inStr,
        static_cast<int>(inStrLen),
        &outBuf[0],
        outStrLen,
        NULL,
        NULL);
    if (result == 0)
        return "";
    return std::string(&outBuf[0]);
#else
    // std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
    // return cvt.to_bytes(u16Str);
    // setlocale(LC_ALL, "");
    // char* locstr = setlocale(LC_CTYPE, NULL);
    // char* encoding = nl_langinfo(CODESET);
    char dest_str[100];
    char* out = dest_str;
    char* in = (char*)u16Str;
    size_t outbytes = sizeof(dest_str);
    std::string result;
    iconv_t conv = iconv_open("UTF-8", "utf-16le");
    if (conv == (iconv_t)-1)
    {
        return std::string();
    }
    while (iconv(conv, &in, &inStrLen, &out, &outbytes) == (size_t)-1)
    {
        if (errno == E2BIG)
        {
            result.append(dest_str, sizeof(dest_str) - outbytes);
            out = dest_str;
            outbytes = sizeof(dest_str);
        }
        else
        {
            iconv_close(conv);
            return std::string();
        }
    }
    result.append(dest_str, sizeof(dest_str) - outbytes);
    iconv_close(conv);
    return result;
#endif
}

inline std::string U16ToU8(const wchar_t* u16Str, size_t inStrLen) {
    return U16ToU8((const char16_t*)u16Str, inStrLen);
}
inline std::string U16ToU8(std::u16string_view strview) {
    return U16ToU8(strview.data(), strview.size());
}

inline std::u16string U8ToU16(const char* u8Str, size_t inStrLen) {
#ifdef WIN32
    if ((u8Str == NULL) || (*u8Str == '\0'))
        return std::u16string();
    int outStrLen = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        u8Str,
        (int)inStrLen,
        NULL,
        0);
    if (outStrLen == 0)
        return std::u16string();

    std::vector<char16_t> outBuf;
    outBuf.assign(outStrLen+1, L'\0');
    int result = ::MultiByteToWideChar(CP_UTF8,
        MB_ERR_INVALID_CHARS,
        u8Str,
        (int)inStrLen,
        (LPWSTR)&outBuf[0],
        outStrLen);
    if (result == 0)
        return std::u16string();

    return std::u16string(&outBuf[0]);
#else
    // std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
    // return cvt.from_bytes(u8Str);
    char dest_str[100];
    char* out = dest_str;
    char* in = (char*)u8Str;
    size_t outbytes = sizeof(dest_str);
    std::u16string result;
    iconv_t conv = iconv_open("UTF-16le", "UTF-8");
    if (conv == (iconv_t)-1)
    {
        return std::u16string();
    }
    while (iconv(conv, &in, &inStrLen, &out, &outbytes) == (size_t)-1)
    {
        if (errno == E2BIG)
        {
            if ((sizeof(dest_str) - outbytes) % 2 != 0)
            {
                iconv_close(conv);
                return std::u16string();
            }
            result.append((char16_t*)dest_str, (sizeof(dest_str) - outbytes) / 2);
            out = dest_str;
            outbytes = sizeof(dest_str);
        }
        else
        {
            iconv_close(conv);
            return std::u16string();
        }
    }
    if ((sizeof(dest_str) - outbytes) % 2 != 0)
    {
        iconv_close(conv);
        return std::u16string();
    }
    result.append((char16_t*)dest_str, (sizeof(dest_str) - outbytes) / 2);
    iconv_close(conv);
    return result;
#endif
}

inline std::u16string U8ToU16(const char8_t* u8Str, size_t inStrLen) {
    return U8ToU16((const char*)u8Str, inStrLen);
}

inline std::u16string U8ToU16(std::u8string_view strview) {
    return U8ToU16(strview.data(), strview.size());
}

inline std::u32string U8ToU32(const char* u8Str) {
    std::u32string out;
    std::mbstate_t state{};
    char32_t c32;
    const char* u8cursor = u8Str;
    const char* u8end = u8Str + strlen(u8Str);

    while (std::size_t rc = std::mbrtoc32(&c32, u8cursor, u8end - u8cursor, &state))
    {
        if (rc == (std::size_t)-1)
            break;
        if (rc == (std::size_t)-2)
            break;
        if (rc == (std::size_t)-3)
            break;
        out.append(1, c32);
        u8cursor += rc;
    }
    return out;
}

inline std::string U32ToU8(const char32_t* u32Str) {
    std::string out;
    std::mbstate_t state{};
    char buf[MB_LEN_MAX]{};
    for (int i = 0; u32Str[i] != 0; i++) {
        std::size_t rc = std::c32rtomb(buf, u32Str[i], &state);
        if (rc == (std::size_t)-1) {
            continue;
        }
        out.append(buf, rc);
    }
    return out;
}

inline std::string U32ToU8(const char32_t* u32Str,size_t num) {
    std::string out;
    std::mbstate_t state{};
    char buf[MB_LEN_MAX]{};
    for (int i = 0; i< num; i++) {
        std::size_t rc = std::c32rtomb(buf, u32Str[i], &state);
        if (rc == (std::size_t)-1) {
            continue;
        }
        out.append(buf, rc);
    }
    return out;
}

constexpr std::string_view ConvertU8StringToView(const std::u8string& str) {
    return std::string_view((const char*)str.c_str(), str.size());
}

constexpr std::u8string_view ConvertStringToU8View(const std::string& str) {
    return std::u8string_view((const char8_t*)str.c_str(), str.size());
}

constexpr std::string ConvertU8ViewToString(std::u8string_view view) {
    return std::string((const char*)view.data(), view.size());
}

constexpr std::u8string_view ConvertViewToU8View(std::string_view view) {
    return std::u8string_view((const char8_t*)view.data(), view.size());
}

constexpr std::string_view ConvertU8ViewToView(std::u8string_view view) {
    return std::string_view((const char*)view.data(), view.size());
}

constexpr std::u16string_view ConvertWViewToU16View(std::wstring_view view) {
    return std::u16string_view((const char16_t*)view.data(), view.size());
}

constexpr std::wstring_view ConvertU16ViewToWView(std::u16string_view view) {
    return std::wstring_view((const wchar_t*)view.data(), view.size());
}

inline char* StrCopy(char* dst,std::string_view view) {
    memcpy(dst, view.data(), view.size());
    dst[view.size()] = '\0';
    return dst;
}