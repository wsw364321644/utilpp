#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "steam_util_export_defs.h"

#define REG_COMMAND_PATH "Shell\\Open\\Command";
#define STEAM_PRODUCT_NAME "steam"
#define STEAM_SERVICE_NAME "Steam Client Service"

STEAM_UTIL_API bool IsSteamClientInstalled();
STEAM_UTIL_API bool IsSteamClientStarted();
///@param[out] path   buf could be null
///@param[in, out] length  in buf length  out string length
STEAM_UTIL_API bool GetSteamClientDir(char* path, uint32_t* length);
STEAM_UTIL_API bool SteamBrowserProtocolRun(uint32_t appid, int argc, const char* const* argv);
STEAM_UTIL_API bool SteamBrowserProtocolInstall(uint32_t appid);