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

namespace sonkwo
{
    const int32_t MAX_GAMENAME = 256;
    const uint32_t INVALID_USERID = 0xffffffffU;
    const uint32_t INVALID_GAMEID = 0;
    const uint32_t INVALID_DLCID = 0;
    const uint32_t INVALID_PATCHID = 0;
    const uint32_t INVALID_INDEX = 0xffffffffU;

    // from client
    const uint32_t MAX_BLOCK_SIZE = 256 * 1024;
    const uint32_t HTTP_BLOCK_SIZE = 4 * 1024 * 1024;
    const uint32_t HTTP_BLOCK_COUNT = HTTP_BLOCK_SIZE / MAX_BLOCK_SIZE;
    const uint16_t USERLEVEL_INNAT = 0U;
    const uint16_t USERLEVEL_OUTNAT = 1U;
    const uint16_t USERLEVEL_SUPER = 2U;
    const uint32_t META_VERSION = 1;
    const uint32_t MAX_FILE_COUNT = 64;

    const uint8_t USERSTATE_STOP = 0;		    // 处于挂起状态，不提供上传也不下载
    const uint8_t USERSTATE_START = 1;		    // 处于活动状态，还未下载完毕，可以提供上传也可以下载
    const uint8_t USERSTATE_COMPLETED = 2;		// 处于活动状态，已经下载完毕，可以提供上传
    const uint32_t DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024;


    inline uint64_t MakeTaskKey(uint32_t content, uint32_t type)
    {
        uint64_t key = content;
        return ((key << 32) | type);
    }

    const uint32_t DEF_FILE_QUEUE_SIZE = 1;
    const uint32_t DEF_HTTP_QUEUE_SIZE = 1;
    const uint32_t DEF_TM_INTERVAL = 100;
}
