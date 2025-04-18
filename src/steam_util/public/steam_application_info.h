#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "steam_util_export_defs.h"

constexpr char REG_COMMAND_PATH[] = "Shell\\Open\\Command";;
constexpr char STEAM_PRODUCT_NAME[] = "steam";
constexpr char STEAM_SERVICE_NAME[] = "Steam Client Service";
constexpr char STEAM_APP_MANIFEST_FORMAT_STRING[] = "appmanifest_{}.acf";
constexpr char STEAM_APP_MANIFEST_REGEX_STRING[] = R"(appmanifest_([0-9]+)\.acf)";

constexpr char STEAMAPPS_FOLDER_NAME[] = "steamapps";
constexpr char LIBRARY_FOLDERS_FILE_NAME[] = "libraryfolders.vdf";
constexpr char LIBRARY_FOLDERS_FILE_REGEX_STRING[] = R"(libraryfolders\.vdf)";

constexpr uint32_t STEAM_APP_FLAG_INSTALLED = 1 << 3;
constexpr uint32_t STEAM_APP_FLAG_NEED_UPDATE = 1 << 1;
constexpr uint32_t STEAM_APP_FLAG_UPDATING = 1 << 10;
constexpr uint32_t STEAM_APP_FLAG_NOT_INSTALL = 1 << 9;

STEAM_UTIL_API bool IsSteamClientInstalled();
STEAM_UTIL_API bool IsSteamClientStarted();
///@param[out] path   buf could be null
///@param[in, out] length  in buf length  out string length
STEAM_UTIL_API bool GetSteamClientPath(char* path, uint32_t* length);
STEAM_UTIL_API bool SteamBrowserProtocolRun(uint32_t appid, int argc, const char* const* argv);
STEAM_UTIL_API bool SteamBrowserProtocolInstall(uint32_t appid);