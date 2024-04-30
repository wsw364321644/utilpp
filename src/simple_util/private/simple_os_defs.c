#include "simple_os_defs.h"

#ifdef WIN32
const uint32_t UTIL_CREATE_ALWAYS = 2;//CREATE_ALWAYS;
const uint32_t UTIL_CREATE_NEW = 1;// CREATE_NEW;
const uint32_t UTIL_OPEN_ALWAYS = 4;// OPEN_ALWAYS;
const uint32_t UTIL_OPEN_EXISTING = 3;// OPEN_EXISTING;
const uint32_t UTIL_TRUNCATE_EXISTING = 5;// TRUNCATE_EXISTING;
#else
const uint32_t UTIL_CREATE_ALWAYS = O_RDWR | O_CREAT | O_TRUNC;
const uint32_t UTIL_OPEN_ALWAYS = O_RDWR | O_CREAT;
const uint32_t UTIL_OPEN_EXISTING = O_RDWR;
#endif