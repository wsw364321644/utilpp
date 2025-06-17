#pragma once
#ifdef STEAM_UTIL_HAS_NET
#include <stdbool.h>
#include <stdint.h>
#include <HTTP/HttpManager.h>
#include "steam_util_export_defs.h"
/**
* web api rely on IHttpManager
* need call Tick on HttpManagerPtr
*/
namespace utilpp {
    typedef std::function<void(bool, std::u8string_view)> FCommonBodyDelagate;
    constexpr char STEAM_API_HOST[] = "api.steampowered.com";

    typedef std::function<void(bool)> FSteamRegisterKeyDelagate;
    STEAM_UTIL_EXPORT HttpRequestPtr SteamRegisterKey(HttpManagerPtr pHttpManager, std::string_view sessionID, std::string_view key, FSteamRegisterKeyDelagate Delagate);

    /////////////ISteamDirectory
    constexpr char I_STEAM_DIRECTORY_PATH[] = "ISteamDirectory";
    typedef struct EndpointDetail_t {
        std::string Host;
        std::string Type;
        uint16_t Port;
        uint8_t Load;
        float WeightedTotalDemandLoad;
    }EndpointDetail_t;
    typedef struct GetCMListForConnectResp_t {
        STEAM_UTIL_EXPORT bool Parse(std::u8string_view view);
        std::vector<EndpointDetail_t> EndpointDetails;
    }GetCMListForConnectResp_t;
    STEAM_UTIL_EXPORT HttpRequestPtr GetCMListForConnect(HttpManagerPtr pHttpManager, FCommonBodyDelagate Delagate);
}

#endif // STEAM_UTIL_HAS_NET