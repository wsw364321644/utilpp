#pragma once
#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif // __cplusplus


#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <Windows.h>
#endif

#define F_HANDLE HANDLE
#define pid_t DWORD
#define MAX_COMMAND_LINE_LEHGTH 0X7fff

typedef void* (__cdecl *fnmalloc)(size_t _Size);
typedef void(__cdecl* fnfree)(void* const block);

#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <limits.h>
#include <signal.h>

#define F_HANDLE int
#define MAX_COMMAND_LINE_LEHGTH ARG_MAX

typedef void* (*fnmalloc)(size_t _Size);
typedef void(* fnfree)(void* const block);
#endif

#define ERR_SUCCESS			(0)		// success 
#define ERR_FAILED			(-1)	// common failure
#define ERR_ARGUMENT		(-2)	// argument error 
#define ERR_FILE			(-3)	// file operation related
#define ERR_METAPARSE		(-4)	// faile to parse mf file
#define ERR_PENDING		(-5)	// wait data



#ifdef WIN32
#define UTIL_CREATE_ALWAYS  CREATE_ALWAYS//CREATE_ALWAYS;
#define UTIL_CREATE_NEW CREATE_NEW// CREATE_NEW;
#define UTIL_OPEN_ALWAYS OPEN_ALWAYS// OPEN_ALWAYS;
#define UTIL_OPEN_EXISTING OPEN_EXISTING// OPEN_EXISTING;
#define UTIL_TRUNCATE_EXISTING  TRUNCATE_EXISTING// TRUNCATE_EXISTING;
#else
#define UTIL_CREATE_ALWAYS (O_RDWR | O_CREAT | O_TRUNC)
#define UTIL_CREATE_NEW (O_RDWR | O_CREAT | O_EXCL | O_TRUNC)
#define UTIL_OPEN_ALWAYS (O_RDWR | O_CREAT)
#define UTIL_OPEN_EXISTING (O_RDWR)
#define UTIL_TRUNCATE_EXISTING (O_RDWR | O_TRUNC)
#endif