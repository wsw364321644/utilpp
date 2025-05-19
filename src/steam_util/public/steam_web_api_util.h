#pragma once
#ifdef STEAM_UTIL_HAS_NET
#include <stdbool.h>
#include <stdint.h>
#include <HTTP/HttpManager.h>
#include "steam_util_export_defs.h"

typedef std::function<void(bool)> FSteamRegisterKeyDelagate;
STEAM_UTIL_API bool SteamRegisterKey(HttpManagerPtr pHttpManager, std::string_view sessionID,std::string_view key, FSteamRegisterKeyDelagate Delagate);
#endif // STEAM_UTIL_HAS_NET