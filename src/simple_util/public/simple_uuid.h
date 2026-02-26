#pragma once
#include <stdint.h>
#include "simple_export_defs.h"

#define UUID_128_BYTES 16
SIMPLE_UTIL_API void generate_uuid_128(uint8_t* const des);
