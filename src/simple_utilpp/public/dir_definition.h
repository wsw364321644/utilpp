#pragma once

#include <stdint.h>
#include <limits>

constexpr size_t PATH_MAX = std::numeric_limits<int16_t>::max();
#ifdef _WIN32
constexpr char8_t FILE_NAMESPACES[] = u8"\\\\?\\";
constexpr wchar_t FILE_NAMESPACESW[] = L"\\\\?\\";
constexpr size_t PATH_PREFIX_MAX = std::char_traits<char8_t>::length(FILE_NAMESPACES);
#else
constexpr size_t PATH_PREFIX_MAX = 0;
#endif // _WIN32
