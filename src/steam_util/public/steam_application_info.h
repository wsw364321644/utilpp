#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <regex>

#include "steam_util_export_defs.h"

constexpr char STEAM_PRODUCT_NAME[] = "steam";
constexpr char STEAM_SERVICE_NAME[] = "Steam Client Service";


constexpr char STEAMAPPS_FOLDER_NAME[] = "steamapps";
constexpr char STEAM_CONFIG_FOLDER_NAME[] = "config";
constexpr char STEAM_APPCACHE_FOLDER_NAME[] = "appcache";
constexpr char STEAM_LIBRARYCACHE_FOLDER_NAME[] = "librarycache";

constexpr char LIBRARY_FOLDERS_FILE_NAME[] = "libraryfolders.vdf";
constexpr char LIBRARY_FOLDERS_FILE_REGEX_STRING[] = R"(libraryfolders\.vdf)";
constexpr char STEAM_APP_MANIFEST_FORMAT_STRING[] = "appmanifest_{}.acf";
constexpr char STEAM_APP_MANIFEST_REGEX_STRING[] = R"(appmanifest_([0-9]+)\.acf)";
constexpr char STEAM_LOGIN_USERS_FILE_NAME[] = "loginusers.vdf";
constexpr char STEAM_APP_INFO_FILE_NAME[] = "appinfo.vdf";
constexpr char STEAM_CONFIG_FILE_NAME[] = "config.vdf";


constexpr uint32_t STEAM_APP_FLAG_INSTALLED = 1 << 3;
constexpr uint32_t STEAM_APP_FLAG_NEED_UPDATE = 1 << 1;
constexpr uint32_t STEAM_APP_FLAG_UPDATING = 1 << 10;
constexpr uint32_t STEAM_APP_FLAG_NOT_INSTALL = 1 << 9;

STEAM_UTIL_API bool IsSteamClientInstalled();
STEAM_UTIL_API bool IsSteamClientStarted();
///@param[out] path   buf could be null
///@param[in, out] length  in buf length  out string length
STEAM_UTIL_API bool GetSteamClientPath(char* path, uint32_t* length);
STEAM_UTIL_API void CloseSteamClient();
STEAM_UTIL_EXPORT bool SetSteamAutoLoginUser(std::u8string_view name);

STEAM_UTIL_API bool SteamBrowserProtocolRun(uint32_t appid, int argc, const char* const* argv);
STEAM_UTIL_API bool SteamBrowserProtocolInstall(uint32_t appid);



inline std::vector<std::string_view> GetSteamKeysFromText(std::string_view text) {
   std::vector<std::string_view> results;
   std::regex reg(R"(([0-9A-Z]{5}-?){2,4}[0-9A-Z]{5})");  
   std::cmatch match;  
   if (std::regex_search(text.data(), text.data() + text.size(), match, reg)) {  
       for (const auto& subMatch : match) {  
           results.push_back(std::string_view(subMatch.first, subMatch.second));
       }  
       return results;
   }  
   return results;
}