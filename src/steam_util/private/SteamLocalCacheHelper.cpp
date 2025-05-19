#include "SteamLocalCacheHelper.h"
#include "steam_application_info.h"
#include <dir_util.h>
#include <string_convert.h>
#include <FunctionExitHelper.h>
#include <vdf_parser.hpp>
#include <concurrentqueue.h>
#include <filesystem>
#include <set>
#include <mutex>
#include <regex>


class FSteamLocalCacheHelper :public ISteamLocalCacheHelper
{
public:
    FSteamLocalCacheHelper();

    bool StartMonitor() override;
    void StopMonitor() override;

    const FSteamLibraryCacheInfo& GetSteamLibraryCacheInfo() override { return SteamLibraryCacheInfo; }

    void Tick(float delta) override;
    void TickIO(float delta) override;

    std::filesystem::path SteamClientDir;

    FSteamLibraryCacheInfo SteamLibraryCacheInfo;
    std::regex ManifestRegex;
    std::regex FoldersFileRegex;
    //concurrent
    std::atomic<std::shared_ptr<std::once_flag>> StopMonitorOnceFlagAtomic{ nullptr };
    std::atomic<std::shared_ptr<std::once_flag>> StartMonitorOnceFlagAtomic{ nullptr };

    //Queue
    typedef struct ManifestFileChangeEvent_t {
        std::string LibraryPath;
        uint32_t AppID;
        bool bDelete{ false };
    }ManifestFileChangeEvent_t;
    moodycamel::ConcurrentQueue<ManifestFileChangeEvent_t> ManifestFileChangeEventQueue;
    ManifestFileChangeEvent_t ManifestFileChangeEventBuf[100];

    typedef struct AppInfoChangeEvent_t {
        std::string LibraryPath;
        std::variant< uint32_t, std::shared_ptr<SteamLibraryAppInfo_t>> AppInfo;
        bool bDelete{ false };
    }AppInfoChangeEvent_t;
    moodycamel::ConcurrentQueue<AppInfoChangeEvent_t> AppInfoChangeEventQueue;
    AppInfoChangeEvent_t AppInfoChangeEventBuf[100]{};

    moodycamel::ConcurrentQueue<std::shared_ptr<SteamLibraryFolderInfo_t>> NewSteamLibraryFolderInfoQueue;
    moodycamel::ConcurrentQueue<std::u8string_view> RemoveSteamLibraryFolderInfoQueue;

    //slow thread
    std::unordered_map<std::u8string_view, std::shared_ptr<SteamLibraryFolderInfo_t>, string_hash> SteamLibraryFolderInfosCache;
    std::unordered_set<std::u8string_view, string_hash> SteamLibraryFolderRemoveCache;
    std::unordered_map<std::u8string_view, CommonHandle_t, string_hash> SteamLibraryFolderMonitors;

private:
    bool UpdateSteamLibraryFoldersFile(std::u8string_view SteamLibraryFoldersFilePathStr);
    bool UpdateSteamAppManifest(std::shared_ptr<SteamLibraryFolderInfo_t>, uint32_t appid);
    std::shared_ptr<SteamLibraryAppInfo_t> ReadSteamAppManifest(std::u8string_view pathView, uint32_t appid);

    void StartMonitorIO();
    void StopMonitorIO();

    void OnSteamFolderChanged(EFilesystemAction action, std::u8string_view pathView, std::u8string_view pathView2);
};

FSteamLocalCacheHelper::FSteamLocalCacheHelper()
{
    ManifestRegex = std::regex(
        STEAM_APP_MANIFEST_REGEX_STRING,
        std::regex::icase | std::regex::ECMAScript
    );
    FoldersFileRegex = std::regex(
        LIBRARY_FOLDERS_FILE_REGEX_STRING,
        std::regex::icase | std::regex::ECMAScript
    );
}

bool FSteamLocalCacheHelper::StartMonitor()
{
    if (!IsSteamClientInstalled())
    {
        return false;
    }
    char* buf = new char[PATH_MAX];
    FunctionExitHelper_t bufHelper(
        [&] {
            delete[] buf;
        }
    );
    auto len = PATH_MAX;
    if (!GetSteamClientPath(buf, &len)) {
        return false;
    }
    SteamClientDir = std::filesystem::path((const char8_t*)buf).parent_path();

    std::shared_ptr<std::once_flag> expectOnceFlag{ nullptr };
    StartMonitorOnceFlagAtomic.compare_exchange_strong(expectOnceFlag, std::make_unique<std::once_flag>());

    return true;
}

void FSteamLocalCacheHelper::StopMonitor()
{
    std::shared_ptr<std::once_flag> expectOnceFlag{ nullptr };
    StopMonitorOnceFlagAtomic.compare_exchange_strong(expectOnceFlag, std::make_unique<std::once_flag>());
}

bool FSteamLocalCacheHelper::UpdateSteamLibraryFoldersFile(std::u8string_view SteamLibraryFoldersFilePathStr)
{
    std::error_code ec;
    auto SteamLibraryFoldersFilePath = std::filesystem::path(SteamLibraryFoldersFilePathStr);
    if (!std::filesystem::exists(SteamLibraryFoldersFilePath, ec) || ec) {
        return false;
    }
    std::ifstream file(SteamLibraryFoldersFilePath, std::ios_base::binary);
    if (!file.is_open()) {
        return false;
    }
    auto SteamLibrariesRoot = tyti::vdf::read(file);
    SteamLibraryFolderRemoveCache.clear();
    SteamLibraryFolderInfosCache.clear();
    std::transform(SteamLibraryFolderMonitors.cbegin(), SteamLibraryFolderMonitors.cend(),
        std::inserter(SteamLibraryFolderRemoveCache, SteamLibraryFolderRemoveCache.begin()),
        [](const auto& pair)
        { return pair.first; });
    for (auto& [key, value] : SteamLibrariesRoot.childs) {
        tyti::vdf::object& libraryFolderInfoNode = *value;

        auto attribsItr = libraryFolderInfoNode.attribs.find("path");
        if (attribsItr == libraryFolderInfoNode.attribs.end()) {
            continue;
        }
        auto monitorItr = SteamLibraryFolderRemoveCache.find(ConvertStringToU8View(attribsItr->second));
        if (monitorItr != SteamLibraryFolderRemoveCache.end()) {
            SteamLibraryFolderRemoveCache.erase(monitorItr);
            continue;
        }
        std::shared_ptr<SteamLibraryFolderInfo_t> pSteamLibraryFolderInfo = std::make_shared<SteamLibraryFolderInfo_t>();
        pSteamLibraryFolderInfo->Path = attribsItr->second;

        attribsItr = libraryFolderInfoNode.attribs.find("label");
        if (attribsItr == libraryFolderInfoNode.attribs.end()) {
            continue;
        }
        pSteamLibraryFolderInfo->Name = attribsItr->second;

        attribsItr = libraryFolderInfoNode.attribs.find("totalsize");
        if (attribsItr == libraryFolderInfoNode.attribs.end()) {
            continue;
        }
        pSteamLibraryFolderInfo->ToltalSize = std::stoull(attribsItr->second);

        auto childsItr = libraryFolderInfoNode.childs.find("apps");
        if (childsItr == libraryFolderInfoNode.childs.end()) {
            continue;
        }
        auto& appnode = *childsItr->second;
        std::filesystem::path SteamLibraryFolderPath(ConvertStringToU8View(pSteamLibraryFolderInfo->Path));
        for (auto& [appidStr, value] : appnode.attribs) {
            if (!UpdateSteamAppManifest(pSteamLibraryFolderInfo, std::stoul(appidStr))) {
                continue;
            }
        }
        SteamLibraryFolderInfosCache.try_emplace(ConvertStringToU8View(pSteamLibraryFolderInfo->Path), pSteamLibraryFolderInfo);
    }

    for (auto& pathView : SteamLibraryFolderRemoveCache) {
        RemoveSteamLibraryFolderInfoQueue.enqueue(pathView);
        GetFilesystemMonitor()->CancelMonitor(SteamLibraryFolderMonitors[pathView]);
        SteamLibraryFolderMonitors.erase(pathView);
    }

    for (auto& [pathView, pSteamLibraryFolderInfo] : SteamLibraryFolderInfosCache) {
        auto appPath = std::filesystem::path(pathView) / STEAMAPPS_FOLDER_NAME;
        auto MonitorHandle = GetFilesystemMonitor()->Monitor(appPath.u8string(), MONITOR_MASK_MODIFY | MONITOR_MASK_NAME_CHANGE,
            std::bind(&FSteamLocalCacheHelper::OnSteamFolderChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );
        if (MonitorHandle.IsValid()) {
            SteamLibraryFolderMonitors.try_emplace(pathView, MonitorHandle);
        }
        NewSteamLibraryFolderInfoQueue.enqueue(pSteamLibraryFolderInfo);
    }
    return true;
}

bool FSteamLocalCacheHelper::UpdateSteamAppManifest(std::shared_ptr<SteamLibraryFolderInfo_t> pSteamLibraryFolderInfo, uint32_t appid)
{
    auto pSteamLibraryAppInfo = ReadSteamAppManifest(ConvertStringToU8View(pSteamLibraryFolderInfo->Path), appid);
    if (!pSteamLibraryAppInfo) {
        return false;
    }
    pSteamLibraryFolderInfo->Apps.emplace(pSteamLibraryAppInfo->AppID, pSteamLibraryAppInfo);
    return true;

}

std::shared_ptr<SteamLibraryAppInfo_t> FSteamLocalCacheHelper::ReadSteamAppManifest(std::u8string_view pathView, uint32_t appid)
{
    std::filesystem::path SteamAppManifestPath(pathView);
    SteamAppManifestPath /= STEAMAPPS_FOLDER_NAME;
    auto fileNameStr = std::format(STEAM_APP_MANIFEST_FORMAT_STRING, appid);
    SteamAppManifestPath /= fileNameStr;
    std::error_code ec;
    if (!std::filesystem::exists(SteamAppManifestPath, ec) || ec) {
        return nullptr;
    }
    std::ifstream file(SteamAppManifestPath, std::ios_base::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    auto pSteamLibraryAppInfo = std::make_shared<SteamLibraryAppInfo_t>();
    auto SteamAppManifestRoot = tyti::vdf::read(file);
    auto attribsItr = SteamAppManifestRoot.attribs.find("name");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->Name = attribsItr->second;
    }

    attribsItr = SteamAppManifestRoot.attribs.find("appid");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->AppID = std::stol(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("installdir");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->InstallDir = attribsItr->second;
    }

    attribsItr = SteamAppManifestRoot.attribs.find("SizeOnDisk");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->Size = std::stoll(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("StateFlags");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->StateFlags = std::stol(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("LastPlayed");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->LastPlayed = std::stoll(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("BytesToDownload");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->UpdateInfo.BytesDownloaded = std::stoll(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("BytesDownloaded");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->UpdateInfo.BytesDownloaded = std::stoll(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("BytesToStage");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->UpdateInfo.BytesToStage = std::stoll(attribsItr->second);
    }

    attribsItr = SteamAppManifestRoot.attribs.find("BytesStaged");
    if (attribsItr != SteamAppManifestRoot.attribs.end()) {
        pSteamLibraryAppInfo->UpdateInfo.BytesStaged = std::stoll(attribsItr->second);
    }
    return pSteamLibraryAppInfo;
}

void FSteamLocalCacheHelper::StartMonitorIO()
{
    StartMonitorOnceFlagAtomic.store(nullptr);
    auto bRes = UpdateSteamLibraryFoldersFile((SteamClientDir / STEAMAPPS_FOLDER_NAME / LIBRARY_FOLDERS_FILE_NAME).u8string());
    if (!bRes) {
        return;
    }
}

void FSteamLocalCacheHelper::StopMonitorIO()
{
    StopMonitorOnceFlagAtomic.store(nullptr);
}

void FSteamLocalCacheHelper::OnSteamFolderChanged(EFilesystemAction action, std::u8string_view pathView, std::u8string_view pathView2)
{
    //printf("action: %d,path: %s, path2: %s\n", (int)action, (const char*)path.data(), (const char*)path2.data());
    auto FilePathView = pathView;
    switch (action) {
    case EFilesystemAction::FSA_RENAME:
        FilePathView = pathView2;
    }

    std::cmatch matchResult;
    std::filesystem::path filePath(FilePathView);
    filePath = filePath.lexically_normal();
    auto pathStr = filePath.filename().string();
    auto libraryFolderPathStr = filePath.parent_path().parent_path().u8string();
    if (std::regex_search(pathStr.c_str(), matchResult, ManifestRegex)) {
        auto appidStr = matchResult[1].str();
        auto appid = std::stoul(appidStr);
        switch (action) {
        case EFilesystemAction::FSA_RENAME:
        case EFilesystemAction::FSA_CREATE:
        case EFilesystemAction::FSA_MODIFY:
        {
            ManifestFileChangeEventQueue.enqueue(ManifestFileChangeEvent_t{ ConvertU8ViewToString(libraryFolderPathStr), appid });
            break;
        }
        case EFilesystemAction::FSA_DELETE: {
            ManifestFileChangeEventQueue.enqueue(ManifestFileChangeEvent_t{ ConvertU8ViewToString(libraryFolderPathStr), appid ,true });
        }
        }
    }
    else if (std::regex_search(pathStr.c_str(), matchResult, FoldersFileRegex)) {
        switch (action) {
        case EFilesystemAction::FSA_MODIFY:
            std::shared_ptr<std::once_flag> expectOnceFlag{ nullptr };
            StartMonitorOnceFlagAtomic.compare_exchange_strong(expectOnceFlag, std::make_unique<std::once_flag>());
        }
    }
}



void FSteamLocalCacheHelper::Tick(float delta)
{
    std::shared_ptr<SteamLibraryFolderInfo_t> pSteamLibraryFolderInfo;
    while (NewSteamLibraryFolderInfoQueue.try_dequeue(pSteamLibraryFolderInfo)) {
        SteamLibraryCacheInfo.try_emplace(ConvertStringToU8View(pSteamLibraryFolderInfo->Path), pSteamLibraryFolderInfo);
        for (auto& [appID, pSteamLibraryAppInfo] : pSteamLibraryFolderInfo->Apps) {
            TriggerSteamLibraryFolderChangedDelegates(EDataOp::DO_Create,pSteamLibraryFolderInfo);
        }

    }
    std::u8string_view pathToRemoveV;
    while (RemoveSteamLibraryFolderInfoQueue.try_dequeue(pathToRemoveV)) {
        auto libraryItr = SteamLibraryCacheInfo.find(pathToRemoveV);
        if (libraryItr != SteamLibraryCacheInfo.end()) {
            auto& [pathView, pSteamLibraryFolderInfo] = *libraryItr;
            SteamLibraryCacheInfo.erase(libraryItr);
            TriggerSteamLibraryFolderChangedDelegates(EDataOp::DO_Delete, pSteamLibraryFolderInfo);
        }
    }
    do {
        auto num = AppInfoChangeEventQueue.try_dequeue_bulk(AppInfoChangeEventBuf, 100);
        if (num == 0) {
            break;
        }

        for (int i = 0; i < num; i++) {
            AppInfoChangeEvent_t& AppInfoChangeEvent = AppInfoChangeEventBuf[i];
            auto libraryItr = SteamLibraryCacheInfo.find(ConvertStringToU8View(AppInfoChangeEvent.LibraryPath));
            if (libraryItr == SteamLibraryCacheInfo.end()) {
                continue;
            }
            auto& [pathView, pSteamLibraryFolderInfo] = *libraryItr;
            if (AppInfoChangeEvent.bDelete) {
                auto& appID = std::get<uint32_t>(AppInfoChangeEvent.AppInfo);
                auto itr=pSteamLibraryFolderInfo->Apps.find(appID);
                if (itr != pSteamLibraryFolderInfo->Apps.end()) {
                    TriggerSteamAppManifestChangedDelegates(EDataOp::DO_Create, pSteamLibraryFolderInfo, itr->second);
                    pSteamLibraryFolderInfo->Apps.erase(itr);
                }
            }
            else {
                auto& pAppInfo = std::get<std::shared_ptr<SteamLibraryAppInfo_t>>(AppInfoChangeEvent.AppInfo);
                auto[itr,bInsert]=pSteamLibraryFolderInfo->Apps.insert_or_assign(pAppInfo->AppID, pAppInfo);
                if (bInsert) {
                    TriggerSteamAppManifestChangedDelegates(EDataOp::DO_Create, pSteamLibraryFolderInfo, pAppInfo);
                }
                else {
                    TriggerSteamAppManifestChangedDelegates(EDataOp::DO_Update, pSteamLibraryFolderInfo, pAppInfo);
                }
            }
        }
    } while (true);
}

void FSteamLocalCacheHelper::TickIO(float delta)
{
    auto StartMonitorOnceFlag = StartMonitorOnceFlagAtomic.load();
    if (StartMonitorOnceFlag) {
        std::call_once(*StartMonitorOnceFlag, &FSteamLocalCacheHelper::StartMonitorIO, this);
    }
    auto StopMonitorOnceFlag = StopMonitorOnceFlagAtomic.load();
    if (StopMonitorOnceFlag) {
        std::call_once(*StopMonitorOnceFlag, &FSteamLocalCacheHelper::StopMonitorIO, this);
    }

    do {
        auto num = ManifestFileChangeEventQueue.try_dequeue_bulk(ManifestFileChangeEventBuf, 100);
        if (num == 0) {
            break;
        }

        for (int i = 0; i < num; i++) {
            ManifestFileChangeEvent_t& ManifestFileChangeEvent = ManifestFileChangeEventBuf[i];
            if (ManifestFileChangeEvent.bDelete) {
                AppInfoChangeEventQueue.enqueue(AppInfoChangeEvent_t{ std::move(ManifestFileChangeEvent.LibraryPath), ManifestFileChangeEvent.AppID ,true });
                continue;
            }
            auto pSteamLibraryAppInfo = ReadSteamAppManifest(ConvertStringToU8View(ManifestFileChangeEvent.LibraryPath), ManifestFileChangeEvent.AppID);
            if (pSteamLibraryAppInfo) {
                AppInfoChangeEventQueue.enqueue(AppInfoChangeEvent_t{ std::move(ManifestFileChangeEventBuf[i].LibraryPath), pSteamLibraryAppInfo });
            }
        }
    } while (true);
}


ISteamLocalCacheHelper* GetSteamLocalCacheHelperInstance()
{
    static std::atomic<std::shared_ptr<FSteamLocalCacheHelper>> AtomicPtr;
    auto oldptr = AtomicPtr.load();
    if (!oldptr) {
        std::shared_ptr<FSteamLocalCacheHelper> ptr(new FSteamLocalCacheHelper);
        AtomicPtr.compare_exchange_strong(oldptr, ptr);
    }
    return AtomicPtr.load().get();
}