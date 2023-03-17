/**
 * defs.h
 * common definition for shared on all projects
 *
 */

#pragma once
#include <stdint.h>

#ifdef WIN32
#include <Windows.h>
#include <Strsafe.h>
#define F_HANDLE HANDLE
const uint32_t SK_CREATE_ALWAYS = CREATE_ALWAYS;
const uint32_t SK_OPEN_ALWAYS = OPEN_ALWAYS;
const uint32_t SK_OPEN_EXISTING = OPEN_EXISTING;
#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <cstring>
#define F_HANDLE int
const uint32_t SK_CREATE_ALWAYS = O_RDWR | O_CREAT | O_TRUNC;
const uint32_t SK_OPEN_ALWAYS = O_RDWR | O_CREAT;
const uint32_t SK_OPEN_EXISTING = O_RDWR;
#endif

#define ERR_SUCCESS			(0)		// success 
#define ERR_FAILED			(-1)	// common failure
#define ERR_ARGUMENT		(-2)	// argument error 
#define ERR_FILE			(-3)	// file operation related
#define ERR_METAPARSE		(-4)	// faile to parse mf file
typedef void* (*fnmalloc)(size_t _Size);
