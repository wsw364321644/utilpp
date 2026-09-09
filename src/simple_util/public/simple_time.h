#pragma once

#include "simple_export_defs.h"
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
SIMPLE_UTIL_API uint64_t os_gettime_ns(void);
SIMPLE_UTIL_API void timestamp_to_tm(uint64_t ts, struct tm* out);