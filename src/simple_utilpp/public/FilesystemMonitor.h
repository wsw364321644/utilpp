#pragma once
#include "simple_export_defs.h"
#include "handle.h"
#include <string_view>
#include <stdint.h>
enum class EFilesystemAction{
    FSA_ACCESS,         //read file
    FSA_OPEN,           //open file
    FSA_ATTRIB,
    FSA_CREATE,
    FSA_DELETE,
    FSA_RENAME,
    FSA_MODIFY,
};
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_ACCESS;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_OPEN;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_ATTRIB;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_CREATE;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_DELETE;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_RENAME;
SIMPLE_UTIL_API const uint32_t MONITOR_MASK_MODIFY;

SIMPLE_UTIL_API const uint32_t MONITOR_MASK_NAME_CHANGE;

class IFilesystemMonitor
{
public:
    virtual ~IFilesystemMonitor(){};
    typedef std::function<void(EFilesystemAction,std::u8string_view, std::u8string_view)> TMonitorCallback;
    virtual CommonHandle_t Monitor(std::u8string_view path, uint32_t mask, TMonitorCallback) = 0;
    virtual void CancelMonitor(CommonHandle_t handle) = 0;
    virtual void Tick(float delta)=0;
};

SIMPLE_UTIL_API IFilesystemMonitor* GetFilesystemMonitor();