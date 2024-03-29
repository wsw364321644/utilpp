/**
 *  misc.h
 */

#pragma once

#include <string>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#include <Strsafe.h>
#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <cstring>
#endif

inline std::string U16ToU8(const char16_t *u16Str)
{
#ifdef WIN32
    if ((u16Str == NULL) || (*u16Str == L'\0'))
        return "";

    const size_t inStrMax = INT_MAX - 1;
    LPWSTR inStr = (LPWSTR)u16Str;
    size_t inStrLen = 0;
    HRESULT hr = ::StringCchLengthW(inStr, inStrMax, &inStrLen);
    if (FAILED(hr))
        return "";
    // added the terminated 1
    ++inStrLen;
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
    outBuf.assign(outStrLen, '\0');
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
    size_t inbytes = std::char_traits<char16_t>::length(u16Str) * 2;
    char dest_str[100];
    char *out = dest_str;
    char *in = (char *)u16Str;
    size_t outbytes = sizeof(dest_str);
    std::string result;
    iconv_t conv = iconv_open("UTF-8", "utf-16le");
    if (conv == (iconv_t)-1)
    {
        return std::string();
    }
    while (iconv(conv, &in, &inbytes, &out, &outbytes) == (size_t)-1)
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
inline std::u16string U8ToU16(const char *u8Str)
{
#ifdef WIN32
    if ((u8Str == NULL) || (*u8Str == '\0'))
        return std::u16string();

    const size_t inStrMax = INT_MAX - 1;
    size_t inStrLen;
    HRESULT hr = ::StringCchLengthA(u8Str, inStrMax, &inStrLen);
    if (FAILED(hr))
        return std::u16string();

    ++inStrLen; // add size for '\0'

    int outStrLen = ::MultiByteToWideChar(CP_UTF8,
                                          MB_ERR_INVALID_CHARS,
                                          u8Str,
                                          (int)inStrLen,
                                          NULL,
                                          0);
    if (outStrLen == 0)
        return std::u16string();

    std::vector<char16_t> outBuf;
    outBuf.assign(outStrLen, L'\0');
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
    size_t inbytes = std::char_traits<char>::length(u8Str);
    char dest_str[100];
    char *out = dest_str;
    char *in = (char *)u8Str;
    size_t outbytes = sizeof(dest_str);
    std::u16string result;
    iconv_t conv = iconv_open("UTF-16le", "UTF-8");
    if (conv == (iconv_t)-1)
    {
        return std::u16string();
    }
    while (iconv(conv, &in, &inbytes, &out, &outbytes) == (size_t)-1)
    {
        if (errno == E2BIG)
        {
            if ((sizeof(dest_str) - outbytes) % 2 != 0)
            {
                iconv_close(conv);
                return std::u16string();
            }
            result.append((char16_t *)dest_str, (sizeof(dest_str) - outbytes) / 2);
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
    result.append((char16_t *)dest_str, (sizeof(dest_str) - outbytes) / 2);
    iconv_close(conv);
    return result;
#endif
}
