#pragma once
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif // __cplusplus
#include "simple_export_defs.h"

SIMPLE_UTIL_API extern const uint32_t UTIL_CREATE_ALWAYS ;//CREATE_ALWAYS;
SIMPLE_UTIL_API extern const uint32_t UTIL_CREATE_NEW;// CREATE_NEW;
SIMPLE_UTIL_API extern const uint32_t UTIL_OPEN_ALWAYS;// OPEN_ALWAYS;
SIMPLE_UTIL_API extern const uint32_t UTIL_OPEN_EXISTING;// OPEN_EXISTING;
SIMPLE_UTIL_API extern const uint32_t UTIL_TRUNCATE_EXISTING;// TRUNCATE_EXISTING;

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define F_HANDLE  void *

#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <cstring>
#define F_HANDLE int
#endif

#define ERR_SUCCESS			(0)		// success 
#define ERR_FAILED			(-1)	// common failure
#define ERR_ARGUMENT		(-2)	// argument error 
#define ERR_FILE			(-3)	// file operation related
#define ERR_METAPARSE		(-4)	// faile to parse mf file

typedef void* (__cdecl *fnmalloc)(size_t _Size);
typedef void(__cdecl* fnfree)(void* const block);
