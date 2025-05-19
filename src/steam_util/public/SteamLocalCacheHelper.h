#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <std_ext.h>
#include <delegate_macros.h>
#include <handle.h>
#include <data_op_define.h>
#include <FilesystemMonitor.h>
#include "steam_util_export_defs.h"

typedef struct SteamLibraryAppUpdateInfo_t {
    uint64_t BytesToDownload;
    uint64_t BytesDownloaded;
    uint64_t BytesToStage;
    uint64_t BytesStaged;
}SteamLibraryAppUpdateInfo_t;

typedef struct SteamLibraryAppInfo_t
{
    uint32_t AppID;
    uint64_t Size;
    uint64_t LastPlayed;
    uint32_t StateFlags;
    SteamLibraryAppUpdateInfo_t UpdateInfo;
    std::string Name;
    std::string InstallDir;

} SteamLibraryAppInfo_t;

typedef struct SteamLibraryFolderInfo_t
{
    std::unordered_map<uint32_t, std::shared_ptr<SteamLibraryAppInfo_t>> Apps;
    std::string Path;
    std::string Name;
    uint64_t ToltalSize;
} SteamLibraryFolderInfo_t;

class STEAM_UTIL_EXPORT ISteamLocalCacheHelper
{
public:
    typedef std::unordered_map<std::u8string_view, std::shared_ptr<SteamLibraryFolderInfo_t>, string_hash> FSteamLibraryCacheInfo;
    virtual ~ISteamLocalCacheHelper() = default;
    virtual bool StartMonitor() = 0;
    virtual void StopMonitor() = 0;

    virtual const FSteamLibraryCacheInfo& GetSteamLibraryCacheInfo() = 0;
    DEFINE_EVENT_TWO_PARAM(SteamLibraryFolderChanged, EDataOp, std::shared_ptr<const SteamLibraryFolderInfo_t>);
    DEFINE_EVENT_THREE_PARAM(SteamAppManifestChanged, EDataOp, std::shared_ptr<const SteamLibraryFolderInfo_t>, std::shared_ptr<const SteamLibraryAppInfo_t>);

    virtual void Tick(float delta) = 0;
    virtual void TickIO(float delta) = 0;
};

STEAM_UTIL_EXPORT ISteamLocalCacheHelper* GetSteamLocalCacheHelperInstance();