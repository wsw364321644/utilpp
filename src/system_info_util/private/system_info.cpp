#include "system_info.h"
#include "system_info_inner.h"
#include <std_ext.h>
#include <string_convert.h>

#include <simdjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstring>
#include <memory>

#ifdef HAS_CPUID
#include <libcpuid.h>
#endif 
namespace utilpp {

    // CPU 信息
    void GetCpuInfo(CpuInfo_t& info)
    {
#ifdef HAS_CPUID
        if (!cpuid_present()) {
            GetCpuInfoPlat(info);
        }
        struct cpu_raw_data_t raw;                                             // contains only raw data
        struct cpu_id_t data;                                                  // contains recognized CPU features data

        if (cpuid_get_raw_data(&raw) < 0) {                                    // obtain the raw CPUID data
            GetCpuInfoPlat(info);
        }
        if (cpu_identify(&raw, &data) < 0) {                                   // identify the CPU, using the given raw data.
            GetCpuInfoPlat(info);
        }

        int CPUInfo[4] = { -1 };
        unsigned   nExIds, i = 0;

        strcpy(info.ProcessorBrandString, data.brand_str);

        strcpy(info.IdentificationString, data.vendor_str);


        info.CoreNum = data.num_cores;
        info.LogicalCoreNum = data.num_logical_cpus;

        msr_driver_t* driver;
        driver = cpu_msr_driver_open();

        if (driver) {
            info.ProcessorBaseFrequencyMHz = cpu_msrinfo(driver, cpu_msrinfo_request_t::INFO_APERF);
            info.MaximumFrequencyMHz = cpu_msrinfo(driver, cpu_msrinfo_request_t::INFO_MPERF);
            info.BusFrequencyMHz = cpu_msrinfo(driver, cpu_msrinfo_request_t::INFO_BUS_CLOCK);
            cpu_msr_driver_close(driver);
        }
        else {
            info.MaximumFrequencyMHz = info.ProcessorBaseFrequencyMHz = cpu_clock();
            info.BusFrequencyMHz = 0;
        }
#endif
    }



    void GetSysInfo(SystemInfo_t& info)
    {

        GetOsInfo(info.OSInfo);
        GetMemStatus(info.MemInfo);
        GetCpuInfo(info.CpuInfo);
        GetDiskInfos(info.DriverInfos);
        GetDisplayInfos(info.DisplayInfos);
        GetBIOSInfo(info.BIOSInfo);
        GetVideoControllerInfos(info.VideoControllerInfos);
        return;
    }


//    NLOHMANN_JSON_SERIALIZE_ENUM(EDriverType, {
//{EDriverType::E_DRIVE_UNKNOWN, ToString(EDriverType::E_DRIVE_UNKNOWN)},
//{EDriverType::E_DRIVE_NO_ROOT_DIR, ToString(EDriverType::E_DRIVE_NO_ROOT_DIR)},
//{EDriverType::E_DRIVE_REMOVABLE, ToString(EDriverType::E_DRIVE_REMOVABLE)},
//{EDriverType::E_DRIVE_FIXED, ToString(EDriverType::E_DRIVE_FIXED)},
//{EDriverType::E_DRIVE_REMOTE, ToString(EDriverType::E_DRIVE_REMOTE)},
//{EDriverType::E_DRIVE_CDROM, ToString(EDriverType::E_DRIVE_CDROM)},
//{EDriverType::E_DRIVE_RAMDISK, ToString(EDriverType::E_DRIVE_RAMDISK)},
//        })
//
//
//        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LogicalDriverInfo_t, DriverType, FreeBytesToCaller, TotalBytes, FreeBytes);
//    void to_json(nlohmann::json& j, const CpuInfo_t& info) {
//        j["ProcessorBrandString"] = info.ProcessorBrandString;
//        j["IdentificationString"] = info.IdentificationString;
//        j["ProcessorBaseFrequencyMHz"] = info.ProcessorBaseFrequencyMHz;
//        j["MaximumFrequencyMHz"] = info.MaximumFrequencyMHz;
//        j["BusFrequencyMHz"] = info.BusFrequencyMHz;
//        j["LogicalCoreNum"] = info.LogicalCoreNum;
//        j["CoreNum"] = info.CoreNum;
//    }
//    void from_json(const nlohmann::json& j, CpuInfo_t& info) {
//        strcpy(info.ProcessorBrandString, j.at("ProcessorBrandString").get_ref<const std::string&>().c_str());
//        strcpy(info.IdentificationString, j.at("IdentificationString").get_ref<const std::string&>().c_str());
//        j.at("ProcessorBaseFrequencyMHz").get_to(info.ProcessorBaseFrequencyMHz);
//        j.at("MaximumFrequencyMHz").get_to(info.MaximumFrequencyMHz);
//        j.at("BusFrequencyMHz").get_to(info.BusFrequencyMHz);
//        j.at("LogicalCoreNum").get_to(info.LogicalCoreNum);
//        j.at("CoreNum").get_to(info.CoreNum);
//    }
//
//    void to_json(nlohmann::json& j, const OSInfo& info) {
//        j["OSName"] = info.OSName;
//        j["DeviceGUID"] = info.DeviceGUID;
//    }
//    void from_json(const nlohmann::json& j, OSInfo& info) {
//        strcpy(info.OSName, j.at("OSName").get_ref<const std::string&>().c_str());
//        strcpy(info.DeviceGUID, j.at("DeviceGUID").get_ref<const std::string&>().c_str());
//    }
//
//    void to_json(nlohmann::json& j, const BIOSInfo_t& info) {
//        j["BaseBoardProduct"] = info.BaseBoardProduct;
//        j["BaseBoardManufacturer"] = info.BaseBoardManufacturer;
//    }
//    void from_json(const nlohmann::json& j, BIOSInfo_t& info) {
//        strcpy(info.BaseBoardProduct, j.at("BaseBoardProduct").get_ref<const std::string&>().c_str());
//        strcpy(info.BaseBoardManufacturer, j.at("BaseBoardManufacturer").get_ref<const std::string&>().c_str());
//    }
//
//    void to_json(nlohmann::json& j, const PhysicalMemoryInfo_t& info) {
//        j["Manufacturer"] = info.Manufacturer;
//        j["TotalBytes"] = info.TotalBytes;
//        j["Speed"] = info.Speed;
//        j["SMBIOSMemoryType"] = info.SMBIOSMemoryType;
//    }
//    void from_json(const nlohmann::json& j, PhysicalMemoryInfo_t& info) {
//        strcpy(info.Manufacturer, j.at("Manufacturer").get_ref<const std::string&>().c_str());
//        j.at("TotalBytes").get_to(info.TotalBytes);
//        j.at("Speed").get_to(info.Speed);
//        j.at("SMBIOSMemoryType").get_to(info.SMBIOSMemoryType);
//    }
//
//    void to_json(nlohmann::json& j, const DriverInfo_t& info) {
//        j["Model"] = info.Model;
//        j["TotalBytes"] = info.TotalBytes;
//    }
//    void from_json(const nlohmann::json& j, DriverInfo_t& info) {
//        strcpy(info.Model, j.at("Model").get_ref<const std::string&>().c_str());
//        j.at("TotalBytes").get_to(info.TotalBytes);
//    }
//
//    void to_json(nlohmann::json& j, const DisplayInfo& info) {
//        j["DisplayName"] = info.DisplayName;
//    }
//    void from_json(const nlohmann::json& j, DisplayInfo& info) {
//        strcpy(info.DisplayName, j.at("DisplayName").get_ref<const std::string&>().c_str());
//    }
//
//    void to_json(nlohmann::json& j, const VideoControllerInfo_t& info) {
//        j["Name"] = info.Name;
//    }
//    void from_json(const nlohmann::json& j, VideoControllerInfo_t& info) {
//        strcpy(info.Name, j.at("Name").get_ref<const std::string&>().c_str());
//    }
//    void to_json(nlohmann::json& j, const MemInfo_t& info) {
//        j["TotalBytes"] = info.TotalBytes;
//        j["MemoryLoad"] = info.MemoryLoad;
//        nlohmann::json arrnode;
//        for (int i = 0; i < info.Num; i++) {
//            nlohmann::json node = info.PhysicalMemoryInfos[i];
//            arrnode.push_back(node);
//        }
//        j["PhysicalMemoryInfos"] = arrnode;
//    }
//
//    void from_json(const nlohmann::json& j, MemInfo_t& info) {
//        j.at("TotalBytes").get_to(info.TotalBytes);
//        j.at("MemoryLoad").get_to(info.MemoryLoad);
//        j.at("MemoryLoad").get_to(info.MemoryLoad);
//        info.Num = 0;
//        for (auto itr : j.at("PhysicalMemoryInfos")) {
//            info.PhysicalMemoryInfos[info.Num] = itr.get<PhysicalMemoryInfo_t>();
//            info.Num++;
//        }
//    }
//
//    void to_json(nlohmann::json& j, const DriverInfos_t& info) {
//        nlohmann::json arrnode;
//        for (int i = 0; i < info.Num; i++) {
//            nlohmann::json node = info.Drivers[i];
//            arrnode.push_back(node);
//        }
//        j["Drivers"] = arrnode;
//        arrnode.clear();
//        for (int i = 0; i < info.NumLogical; i++) {
//            nlohmann::json node = info.LogicalDrivers[i];
//            arrnode.push_back(node);
//        }
//        j["LogicalDrivers"] = arrnode;
//    }
//
//    void from_json(const nlohmann::json& j, DriverInfos_t& info) {
//        info.Num = 0;
//        if (j.contains("Drivers")) {
//            for (auto itr : j.at("Drivers")) {
//                info.Drivers[info.Num] = itr.get<DriverInfo_t>();
//                info.Num++;
//            }
//        }
//        info.NumLogical = 0;
//        if (j.contains("LogicalDrivers")) {
//            for (auto itr : j.at("LogicalDrivers")) {
//                info.LogicalDrivers[info.NumLogical] = itr.get<LogicalDriverInfo_t>();
//                info.NumLogical++;
//            }
//        }
//    }
//
//    void to_json(nlohmann::json& j, const DisplayInfos_t& info) {
//        nlohmann::json arrnode;
//        for (int i = 0; i < info.num; i++) {
//            nlohmann::json node = info.Displays[i];
//            arrnode.push_back(node);
//        }
//        j["Displays"] = arrnode;
//    }
//
//    void from_json(const nlohmann::json& j, DisplayInfos_t& info) {
//        info.num = 0;
//        if (j.contains("Displays")) {
//            for (auto itr : j.at("Displays")) {
//                info.Displays[info.num] = itr.get<DisplayInfo_t>();
//                info.num++;
//            }
//        }
//    }
//
//    void to_json(nlohmann::json& j, const VideoControllerInfos_t& info) {
//        nlohmann::json arrnode;
//        for (int i = 0; i < info.num; i++) {
//            if (info.Controllers[i].Availability != 3) {
//                continue;
//            }
//            nlohmann::json node = info.Controllers[i];
//            arrnode.push_back(node);
//        }
//        j["Controllers"] = arrnode;
//    }
//
//    void from_json(const nlohmann::json& j, VideoControllerInfos_t& info) {
//        info.num = 0;
//        if (j.contains("Controllers")) {
//            for (auto itr : j.at("Controllers")) {
//                info.Controllers[info.num] = itr.get<VideoControllerInfo_t>();
//                info.num++;
//            }
//        }
//    }
//    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SystemInfo_t, CpuInfo, OSInfo, BIOSInfo, MemInfo, DriverInfos, DisplayInfos, VideoControllerInfos);
//
//    char* SysInfoToString(SystemInfo_t& info, fnmalloc fn)
//    {
//        nlohmann::json doc;
//        //doc["CpuInfo"] = info.CpuInfo;
//        //doc["OSInfo"] = info.OSInfo;
//        //doc["BIOSInfo"] = info.BIOSInfo;
//        //doc["MemInfo"] =info.MemInfo;
//        //doc["DriverInfos"] =info.DriverInfos;
//        //doc["DisplayInfos"] =info.DisplayInfos;
//        //doc["VideoControllerInfos"] =info.VideoControllerInfos;
//        doc = info;
//        auto str = doc.dump();
//        char* outstr = (char*)fn(str.size() + 1);
//        strcpy(outstr, str.c_str());
//        return outstr;
//    }
//    bool StringToSysInfo(const char* str, SystemInfo_t& info) {
//        nlohmann::json doc = nlohmann::json::parse(str, nullptr, false);
//        if (doc.is_discarded()) {
//            return false;
//        }
//        info = doc.get< SystemInfo_t>();
//        return true;
//    }

    void CpuInfoToJson(CpuInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("ProcessorBrandString", rapidjson::Value(info.ProcessorBrandString,a), a);
        obj.AddMember("IdentificationString", rapidjson::Value(info.IdentificationString, a), a);
        obj.AddMember("ProcessorBaseFrequencyMHz", info.ProcessorBaseFrequencyMHz, a);
        obj.AddMember("MaximumFrequencyMHz", info.MaximumFrequencyMHz, a);
        obj.AddMember("BusFrequencyMHz", info.BusFrequencyMHz, a);
        obj.AddMember("LogicalCoreNum", info.LogicalCoreNum, a);
        obj.AddMember("CoreNum", info.CoreNum, a);
    }
    void CpuInfoFromJson(simdjson::ondemand::object obj, CpuInfo_t& info)
    {
        auto strres = obj["ProcessorBrandString"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.ProcessorBrandString, strres.value_unsafe());
        strres = obj["IdentificationString"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.IdentificationString, strres.value_unsafe());
        auto uintres = obj["ProcessorBaseFrequencyMHz"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.ProcessorBaseFrequencyMHz = uintres.value_unsafe();
        uintres = obj["MaximumFrequencyMHz"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.MaximumFrequencyMHz = uintres.value_unsafe();
        uintres = obj["BusFrequencyMHz"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.BusFrequencyMHz = uintres.value_unsafe();
        uintres = obj["LogicalCoreNum"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.LogicalCoreNum = uintres.value_unsafe();
        uintres = obj["CoreNum"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.CoreNum = uintres.value_unsafe();
    }

    void OSInfoToJson(OSInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("OSName", rapidjson::Value(info.OSName, a), a);
        obj.AddMember("DeviceGUID", rapidjson::Value(info.DeviceGUID, a), a);
    }
    void OSInfoFromJson(simdjson::ondemand::object obj, OSInfo_t& info)
    {
        auto strres = obj["OSName"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.OSName, strres.value_unsafe());
        strres = obj["DeviceGUID"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.DeviceGUID, strres.value_unsafe());
    }

    void BIOSInfoToJson(BIOSInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("BaseBoardProduct", rapidjson::Value(info.BaseBoardProduct, a), a);
        obj.AddMember("BaseBoardManufacturer", rapidjson::Value(info.BaseBoardManufacturer, a), a);
    }
    void BIOSInfoFromJson(simdjson::ondemand::object obj, BIOSInfo_t& info)
    {
        auto strres = obj["BaseBoardProduct"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.BaseBoardProduct, strres.value_unsafe());
        strres = obj["BaseBoardManufacturer"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.BaseBoardManufacturer, strres.value_unsafe());
    }

    void PhysicalMemoryInfoToJson(PhysicalMemoryInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("Manufacturer", rapidjson::Value(info.Manufacturer, a), a);
        obj.AddMember("TotalBytes", info.TotalBytes, a);
        obj.AddMember("Speed", info.Speed, a);
        obj.AddMember("SMBIOSMemoryType", info.SMBIOSMemoryType, a);
    }
    void PhysicalMemoryInfoFromJson(simdjson::ondemand::object obj, PhysicalMemoryInfo_t& info)
    {
        auto strres = obj["Manufacturer"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.Manufacturer, strres.value_unsafe());
        auto uintres = obj["TotalBytes"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.TotalBytes = uintres.value_unsafe();
        uintres = obj["Speed"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.Speed = uintres.value_unsafe();
        uintres = obj["SMBIOSMemoryType"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.SMBIOSMemoryType = uintres.value_unsafe();
    }
    void MemInfoToJson(MemInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("TotalBytes", info.TotalBytes, a);
        obj.AddMember("MemoryLoad", info.MemoryLoad, a);
        rapidjson::Document arr(rapidjson::kArrayType, &a);
        for (int i = 0; i < info.Num;i++) {
            rapidjson::Document arrItem(rapidjson::kObjectType, &a);
            PhysicalMemoryInfoToJson(info.PhysicalMemoryInfos[i], arrItem);
            arr.PushBack(arrItem, a);
        }
        obj.AddMember("PhysicalMemoryInfos", arr, a);
    }
    void MemInfoFromJson(simdjson::ondemand::object obj, MemInfo_t& info)
    {
        auto uintres = obj["TotalBytes"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.TotalBytes = uintres.value_unsafe();
        uintres = obj["MemoryLoad"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.MemoryLoad = uintres.value_unsafe();
        auto arrres = obj["PhysicalMemoryInfos"].get_array();
        if (arrres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.Num = 0;
        for (auto itr : arrres) {
            auto objres = itr.get_object();
            if (objres.error() != simdjson::error_code::SUCCESS) {
                return;
            }
            PhysicalMemoryInfoFromJson(objres.value_unsafe(), info.PhysicalMemoryInfos[info.Num++]);
        }
    }

    void DriverInfoToJson(DriverInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("Model", rapidjson::Value(info.Model, a), a);
        obj.AddMember("TotalBytes", info.TotalBytes, a);
    }
    void DriverInfoFromJson(simdjson::ondemand::object obj, DriverInfo_t& info)
    {
        auto strres = obj["Model"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.Model, strres.value_unsafe());
        auto uintres = obj["TotalBytes"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.TotalBytes = uintres.value_unsafe();
    }
    void LogicalDriverInfoToJson(LogicalDriverInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("DriverType", std::to_underlying(info.DriverType), a);
        obj.AddMember("FreeBytesToCaller", info.FreeBytesToCaller, a);
        obj.AddMember("TotalBytes", info.TotalBytes, a);
        obj.AddMember("FreeBytes", info.FreeBytes, a);
    }
    void LogicalDriverInfoFromJson(simdjson::ondemand::object obj, LogicalDriverInfo_t& info)
    {
        auto uintres = obj["DriverType"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.DriverType = EDriverType(uintres.value_unsafe());
        uintres = obj["FreeBytesToCaller"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.FreeBytesToCaller = uintres.value_unsafe();
        uintres = obj["TotalBytes"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.TotalBytes = uintres.value_unsafe();
        uintres = obj["FreeBytes"].get_uint64();
        if (uintres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.FreeBytes = uintres.value_unsafe();
    }
    void DriverInfosToJson(DriverInfos_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        rapidjson::Document arr(rapidjson::kArrayType, &a);
        for (int i = 0; i < info.Num;i++) {
            rapidjson::Document arrItem(rapidjson::kObjectType, &a);
            DriverInfoToJson(info.Drivers[i], arrItem);
            arr.PushBack(arrItem, a);
        }
        obj.AddMember("Drivers", arr, a);
        arr.SetArray();
        for (int i = 0; i < info.NumLogical; i++) {
            rapidjson::Document arrItem(rapidjson::kObjectType, &a);
            LogicalDriverInfoToJson(info.LogicalDrivers[i], arrItem);
            arr.PushBack(arrItem, a);
        }
        obj.AddMember("LogicalDrivers", arr, a);
    }
    void DriverInfosFromJson(simdjson::ondemand::object obj ,DriverInfos_t& info)
    {
        auto arrres = obj["Drivers"].get_array();
        if (arrres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.Num = 0;
        for (auto itr : arrres) {
            auto objres = itr.get_object();
            if (objres.error() != simdjson::error_code::SUCCESS) {
                return;
            }
            DriverInfoFromJson(objres.value_unsafe(), info.Drivers[info.Num++]);
        }
        arrres = obj["LogicalDrivers"].get_array();
        if (arrres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.NumLogical = 0;
        for (auto itr : arrres) {
            auto objres = itr.get_object();
            if (objres.error() != simdjson::error_code::SUCCESS) {
                return;
            }
            LogicalDriverInfoFromJson(objres.value_unsafe(), info.LogicalDrivers[info.NumLogical++]);
        }
    }

    void DisplayInfoToJson(DisplayInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("DisplayName", rapidjson::Value(info.DisplayName, a), a);
    }
    void DisplayInfoFromJson(simdjson::ondemand::object obj, DisplayInfo_t& info)
    {
        auto strres = obj["DisplayName"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.DisplayName, strres.value_unsafe());
    }
    void DisplayInfosToJson(DisplayInfos_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        rapidjson::Document arr(rapidjson::kArrayType, &a);
        for (int i = 0; i < info.Num; i++) {
            rapidjson::Document arrItem(rapidjson::kObjectType, &a);
            DisplayInfoToJson(info.Displays[i], arrItem);
            arr.PushBack(arrItem, a);
        }
        obj.AddMember("Displays", arr, a);
    }
    void DisplayInfosFromJson(simdjson::ondemand::object obj, DisplayInfos_t& info)
    {
        auto arrres = obj["Displays"].get_array();
        if (arrres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.Num = 0;
        for (auto itr : arrres) {
            auto objres = itr.get_object();
            if (objres.error() != simdjson::error_code::SUCCESS) {
                return;
            }
            DisplayInfoFromJson(objres.value_unsafe(), info.Displays[info.Num++]);
        }
    }

    void VideoControllerInfoToJson(VideoControllerInfo_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        obj.AddMember("Name", rapidjson::Value(info.Name, a), a);
    }
    void VideoControllerInfoFromJson(simdjson::ondemand::object obj, VideoControllerInfo_t& info)
    {
        auto strres = obj["Name"].get_string();
        if (strres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        StrCopy(info.Name, strres.value_unsafe());
    }
    void VideoControllerInfosToJson(VideoControllerInfos_t& info, rapidjson::Document& obj)
    {
        auto& a = obj.GetAllocator();
        rapidjson::Document arr(rapidjson::kArrayType, &a);
        for (int i = 0; i < info.Num; i++) {
            rapidjson::Document arrItem(rapidjson::kObjectType, &a);
            VideoControllerInfoToJson(info.Controllers[i], arrItem);
            arr.PushBack(arrItem, a);
        }
        obj.AddMember("Controllers", arr, a);
    }
    void VideoControllerInfosFromJson(simdjson::ondemand::object obj, VideoControllerInfos_t& info)
    {
        auto arrres = obj["Controllers"].get_array();
        if (arrres.error() != simdjson::error_code::SUCCESS) {
            return;
        }
        info.Num = 0;
        for (auto itr : arrres) {
            auto objres = itr.get_object();
            if (objres.error() != simdjson::error_code::SUCCESS) {
                return;
            }
            VideoControllerInfoFromJson(objres.value_unsafe(), info.Controllers[info.Num++]);
        }
    }

    void SysInfoToString(SystemInfo_t& info, FCharBuffer& buf)
    {
        rapidjson::Writer<FCharBuffer> writer(buf);
        rapidjson::Document doc(rapidjson::kObjectType);
        auto& a = doc.GetAllocator();
        rapidjson::Document obj(rapidjson::kObjectType,&a);
        CpuInfoToJson(info.CpuInfo, obj);
        doc.AddMember("CpuInfo", obj, a);
        obj.SetObject();
        OSInfoToJson(info.OSInfo, obj);
        doc.AddMember("OSInfo", obj, a);
        obj.SetObject();
        BIOSInfoToJson(info.BIOSInfo, obj);
        doc.AddMember("BIOSInfo", obj, a);
        obj.SetObject();
        MemInfoToJson(info.MemInfo, obj);
        doc.AddMember("MemInfo", obj, a);
        obj.SetObject();
        DriverInfosToJson(info.DriverInfos, obj);
        doc.AddMember("DriverInfos", obj, a);
        obj.SetObject();
        DisplayInfosToJson(info.DisplayInfos, obj);
        doc.AddMember("DisplayInfos", obj, a);
        obj.SetObject();
        VideoControllerInfosToJson(info.VideoControllerInfos, obj);
        doc.AddMember("VideoControllerInfos", obj, a);
        if (!doc.Accept(writer)) {
        }
        return;
    }
    bool StringToSysInfo(const char* str, SystemInfo_t& info) {
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document doc = parser.iterate(str);
        auto objres=doc["CpuInfo"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        CpuInfoFromJson(objres.value_unsafe(), info.CpuInfo);
        objres = doc["OSInfo"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        OSInfoFromJson(objres.value_unsafe(), info.OSInfo);
        objres = doc["BIOSInfo"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        BIOSInfoFromJson(objres.value_unsafe(), info.BIOSInfo);
        objres = doc["MemInfo"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        MemInfoFromJson(objres.value_unsafe(), info.MemInfo);
        objres = doc["DriverInfos"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        DriverInfosFromJson(objres.value_unsafe(), info.DriverInfos);
        objres = doc["DisplayInfos"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        DisplayInfosFromJson(objres.value_unsafe(), info.DisplayInfos);
        objres = doc["VideoControllerInfos"].get_object();
        if (objres.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        VideoControllerInfosFromJson(objres.value_unsafe(), info.VideoControllerInfos);
        return true;
    }




}