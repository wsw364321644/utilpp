#pragma once
#include "simple_export_defs.h"
#include <stdint.h>
#ifdef WIN32
SIMPLE_UTIL_API uint64_t htobe64(uint64_t host_64bits);

#else
#include <endian.h>
#include <arpa/inet.h>
#endif
