#include "FilesystemMonitor.h"


const uint32_t MONITOR_MASK_ACCESS = 1 << 0;
const uint32_t MONITOR_MASK_OPEN = 1 << 1;
const uint32_t MONITOR_MASK_ATTRIB = 1 << 2;
const uint32_t MONITOR_MASK_CREATE = 1 << 3;
const uint32_t MONITOR_MASK_DELETE = 1 << 4;
const uint32_t MONITOR_MASK_RENAME = 1 << 5;
const uint32_t MONITOR_MASK_MODIFY = 1 << 6;
const uint32_t MONITOR_MASK_NAME_CHANGE = MONITOR_MASK_CREATE| MONITOR_MASK_DELETE| MONITOR_MASK_RENAME;

