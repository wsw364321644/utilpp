#pragma once

#include <stdint.h>
#include <string_buffer.h>
#include "system_info_export_defs.h"

namespace utilpp {
    #define VENDOR_STR_MAX 16
    #define BRAND_STR_MAX 64

    #define OS_NAME_MAX 32
    #define OS_GUIDLEN 39

    #define DISPLAY_NAME_MAX 129
    #define DISPLAY_NUM_MAX 16

    #define SYS_PRODUCT_MAX 128
    #define SYS_MANUFACTURER_MAX 128

    #define LOGICAL_DRIVE_NAME_MAX 5
    #define DRIVER_NUM_MAX 16
    #define DRIVER_MODEL_MAX 64

    #define MEM_NUM_MAX 8
    #define MEM_MANUFACTURER_MAX 128
    enum class EDriverType : uint8_t {
        E_DRIVE_UNKNOWN,
        E_DRIVE_NO_ROOT_DIR,
        E_DRIVE_REMOVABLE,
        E_DRIVE_FIXED,
        E_DRIVE_REMOTE,
        E_DRIVE_CDROM,
        E_DRIVE_RAMDISK
    };
    inline const char* ToString(EDriverType v)
    {
        switch (v)
        {
        case EDriverType::E_DRIVE_NO_ROOT_DIR:   return "NO_ROOT_DIR";
        case EDriverType::E_DRIVE_REMOVABLE: return "REMOVABLE";
        case EDriverType::E_DRIVE_FIXED: return "FIXED";
        case EDriverType::E_DRIVE_REMOTE: return "REMOTE";
        case EDriverType::E_DRIVE_CDROM: return "CDROM";
        case EDriverType::E_DRIVE_RAMDISK: return "RAMDISK";
        default:       return "UNKNOWN";
        }
    }
    typedef struct CpuInfo_t {
        char ProcessorBrandString[BRAND_STR_MAX];
        char IdentificationString[VENDOR_STR_MAX];
        uint64_t ProcessorBaseFrequencyMHz;
        uint64_t MaximumFrequencyMHz;
        uint64_t BusFrequencyMHz;
        uint32_t LogicalCoreNum;
        uint32_t CoreNum;
    }CpuInfo_t;

    typedef struct PhysicalMemoryInfo_t {
        char Manufacturer[SYS_MANUFACTURER_MAX];
        uint64_t TotalBytes;
        uint32_t Speed;
        uint32_t SMBIOSMemoryType;
    }PhysicalMemoryInfo_t;
    typedef struct MemInfo_t {
        uint64_t TotalBytes;
        uint8_t MemoryLoad;
        PhysicalMemoryInfo_t PhysicalMemoryInfos[MEM_NUM_MAX];
        uint32_t Num;
    }MemInfo_t;


    typedef struct OSInfo_t {
        char OSName[OS_NAME_MAX];
        char DeviceGUID[OS_GUIDLEN];
    }OSInfo_t;

    typedef struct DisplayInfo_t {
        char DisplayName[DISPLAY_NAME_MAX];
    }DisplayInfo_t;
    typedef struct DisplayInfos_t {
        DisplayInfo_t Displays[DISPLAY_NUM_MAX];
        uint32_t Num;
    }DisplayInfos_t;

    typedef struct LogicalDriverInfo_t {
        char LogicalDriveName[LOGICAL_DRIVE_NAME_MAX];
        EDriverType DriverType;
        uint64_t FreeBytesToCaller;
        uint64_t TotalBytes;
        uint64_t FreeBytes;
    } LogicalDriverInfo_t;

    typedef struct DriverInfo_t {
        char Model[DRIVER_MODEL_MAX];
        uint64_t TotalBytes;
    }DriverInfo_t;

    typedef struct DriverInfos_t {
        LogicalDriverInfo_t LogicalDrivers[DRIVER_NUM_MAX];
        DriverInfo_t Drivers[DRIVER_NUM_MAX];
        uint32_t Num;
        uint32_t NumLogical;
    }DriverInfos_t;

    typedef struct BIOSInfo_t {
        char BaseBoardProduct[SYS_PRODUCT_MAX];
        char BaseBoardManufacturer[SYS_MANUFACTURER_MAX];
    }BIOSInfo_t;

    typedef struct VideoControllerInfo_t {
        char Name[SYS_PRODUCT_MAX];
        uint16_t Availability;
    }VideoControllerInfo_t;
    typedef struct VideoControllerInfos_t {
        VideoControllerInfo_t Controllers[DRIVER_NUM_MAX];
        uint32_t Num;
    }VideoControllerInfos_t;

    typedef struct SystemInfo_t {
        CpuInfo_t CpuInfo;
        OSInfo_t OSInfo;
        BIOSInfo_t BIOSInfo;
        MemInfo_t MemInfo;
        DriverInfos_t DriverInfos;
        DisplayInfos_t DisplayInfos;
        VideoControllerInfos_t VideoControllerInfos;
    }SystemInfo_t;


    //std::string getVideoController();
    SYSTEM_INFO_EXPORT void GetSysInfo(SystemInfo_t& info);

    SYSTEM_INFO_EXPORT void GetMemStatus(MemInfo_t& info);
    SYSTEM_INFO_EXPORT void GetDiskInfos(DriverInfos_t&);
    SYSTEM_INFO_EXPORT void GetOsInfo(OSInfo_t& info);
    SYSTEM_INFO_EXPORT void GetCpuInfo(CpuInfo_t& info);
    SYSTEM_INFO_EXPORT void GetDisplayInfos(DisplayInfos_t& info);
    SYSTEM_INFO_EXPORT void GetBIOSInfo(BIOSInfo_t& info);
    SYSTEM_INFO_EXPORT void GetVideoControllerInfos(VideoControllerInfos_t& info);


    SYSTEM_INFO_EXPORT void SysInfoToString(SystemInfo_t& info, FCharBuffer& buf);
    SYSTEM_INFO_EXPORT bool StringToSysInfo(const char* str, SystemInfo_t& info);
}