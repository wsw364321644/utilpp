#pragma once

#include <simple_os_defs.h>
#include "stdint.h"
#include "stdbool.h"
SIMPLE_UTIL_API size_t bin_to_hex_length(size_t insize);
SIMPLE_UTIL_API size_t hex_to_bin_length(size_t insize);

SIMPLE_UTIL_API bool to_upper_hex(char* const des, const uint8_t* src, size_t insize);
SIMPLE_UTIL_API bool to_lower_hex(char* const des, const uint8_t* src, size_t insize);
SIMPLE_UTIL_API bool hex_to_bin(uint8_t* const des, const char* src, size_t insize);