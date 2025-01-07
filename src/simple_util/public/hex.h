#pragma once

#include <simple_os_defs.h>
#include "stdint.h"

SIMPLE_UTIL_API bool to_upper_hex(char* const des, const uint8_t* src, size_t insize);
SIMPLE_UTIL_API bool to_lower_hex(char* const des, const uint8_t* src, size_t insize);