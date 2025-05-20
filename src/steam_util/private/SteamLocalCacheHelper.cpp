#include "SteamLocalCacheHelper.h"
#include "steam_application_info.h"
#include <dir_util.h>
#include <nlohmann/json.hpp>
#include <string_convert.h>
#include <FunctionExitHelper.h>
#include <vdf_parser.hpp>
#include <concurrentqueue.h>
#include <filesystem>
#include <set>
#include <mutex>
#include <regex>

enum class EMagicNumber :uint32_t {
    MN_V1 = 123094055U,
    MN_V2 = 123094056U,
    MN_V3 = 123094057U,
};
enum class ESteamAppPropertyType :int8_t
{
    SAPT_Invalid_ = -1,
    SAPT_Table = 0,
    SAPT_String = 1,
    SAPT_Int32 = 2,
    SAPT_Float = 3,
    SAPT_WString = 5,
    SAPT_Color = 6,
    SAPT_Uint64 = 7,
    SAPT_EndOfTable = 8,
};

typedef struct SteamAppInfoHeader_t {
    uint32_t MagicNumber{ 0 };
    uint32_t UniveseNumber{ 0 };
}SteamAppInfoHeader_t;
typedef struct SteamAppInfoStringTable_t {
    int64_t StringTableOffset{ 0 };
    uint32_t StringCount{ 0 };
    std::vector<std::string> StringPool;
}SteamAppInfoStringTable_t;
typedef struct SteamAppInfoCache_t {
    uint32_t AppId;
    nlohmann::json PropertyTable{ nlohmann::json::value_t::object};
}SteamAppInfoCache_t;
typedef struct SteamCachedLibrary_t
{
    EMagicNumber GetMagicNumber() {
        return EMagicNumber(Header.MagicNumber);
    }
    bool HasStringTable() {
        return GetMagicNumber() == EMagicNumber::MN_V3;
    }
    bool IsMagicNumberValid() {
        auto MagicNumber = EMagicNumber(Header.MagicNumber);
        switch (MagicNumber) {
        case EMagicNumber::MN_V1:
        case EMagicNumber::MN_V2:
        case EMagicNumber::MN_V3:
            return true;
        default:
            return false;
        }
    }
    SteamAppInfoHeader_t Header;
    SteamAppInfoStringTable_t StringTable;
    std::unordered_map<uint32_t, std::shared_ptr<SteamAppInfoCache_t>> AppInfoCacheMap;
} SteamCachedLibrary_t;



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
        uint32_t AppID{ 0 };
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
    std::shared_ptr<SteamCachedLibrary_t> ReadLibraryCache(std::u8string_view pathView);

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

std::shared_ptr<SteamCachedLibrary_t> FSteamLocalCacheHelper::ReadLibraryCache(std::u8string_view pathView)
{
    std::filesystem::path Path(pathView);
    Path /= STEAM_APPCACHE_FOLDER_NAME;
    Path /= STEAM_APP_INFO_FILE_NAME;
    std::error_code ec;
    if (!std::filesystem::exists(Path, ec) || ec) {
        return nullptr;
    }
    std::ifstream file(Path, std::ios_base::binary);
    std::wifstream wif(Path, std::ios_base::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    if (!wif.is_open()) {
        return nullptr;
    }
    auto pSteamCachedLibrary=std::make_shared<SteamCachedLibrary_t>();
    file.read((char*)&pSteamCachedLibrary->Header, sizeof(pSteamCachedLibrary->Header));
    if (!pSteamCachedLibrary->IsMagicNumberValid()) {
        return nullptr;
    }
    if (pSteamCachedLibrary->HasStringTable()) {
        file.read((char*)&pSteamCachedLibrary->StringTable.StringTableOffset, sizeof(pSteamCachedLibrary->StringTable.StringTableOffset));
        auto pos=file.tellg();
        file.seekg(pSteamCachedLibrary->StringTable.StringTableOffset);
        file.read((char*)&pSteamCachedLibrary->StringTable.StringCount, sizeof(pSteamCachedLibrary->StringTable.StringCount));
        pSteamCachedLibrary->StringTable.StringPool.reserve(pSteamCachedLibrary->StringTable.StringCount);
        for (uint32_t i = 0; i < pSteamCachedLibrary->StringTable.StringCount; i++) {
            std::string& str= pSteamCachedLibrary->StringTable.StringPool[i];
            std::getline(file, str, '\0');
        }
        file.seekg(pos);
    }

    while (true) {
        auto pSteamAppInfo = std::make_shared<SteamAppInfoCache_t>();
        file.read((char*)&pSteamAppInfo->AppId, sizeof(pSteamAppInfo->AppId));
        if (pSteamAppInfo->AppId == 0) {
            break;
        }
        int32_t count;
        file.read((char*)&count, sizeof(count));
        file.seekg(file.tellg() + std::streamoff(16));//stuffBeforeHash
        file.seekg(file.tellg() + std::streamoff(20));//hash
        file.seekg(file.tellg() + std::streamoff(sizeof(uint32_t)));//changeNumber
        if (pSteamCachedLibrary->GetMagicNumber() == EMagicNumber::MN_V2 || pSteamCachedLibrary->GetMagicNumber() == EMagicNumber::MN_V3) {
            file.seekg(file.tellg() + std::streamoff(20));
        }

        std::string StringValue;
        std::wstring WStringValue;
        int32_t int32Value;
        float floatValue;
        uint64_t uint64Value;
        uint8_t uint8Value;
        auto ReadPropertyTable = [&](this const auto& self, nlohmann::json& obj)->void {
            std::string PropertyNameBuf;
            while (true) {
                int8_t type;
                file.read((char*)&type, sizeof(type));
                if (ESteamAppPropertyType(type) == ESteamAppPropertyType::SAPT_EndOfTable) {
                    break;
                }
                std::string* pPropertyName{ nullptr };
                if (pSteamCachedLibrary->HasStringTable()) {
                    int32_t stringIndex;
                    file.read((char*)&stringIndex, sizeof(stringIndex));
                    pPropertyName = &pSteamCachedLibrary->StringTable.StringPool[stringIndex];
                }
                else {
                    std::getline(file, PropertyNameBuf, '\0');
                    pPropertyName = &PropertyNameBuf;
                }

                switch (ESteamAppPropertyType(type))
                {
                case ESteamAppPropertyType::SAPT_Table: {
                    nlohmann::json childObj{ nlohmann::json::value_t::object };
                    self(childObj);
                    obj[*pPropertyName] = childObj;
                    break;
                }
                case ESteamAppPropertyType::SAPT_String: {
                    std::getline(file, StringValue, '\0');
                    obj[*pPropertyName] = StringValue;
                    break;
                }
                case ESteamAppPropertyType::SAPT_WString: {
                    wif.seekg(file.tellg());
                    std::getline(wif, WStringValue, L'\0');
                    obj[*pPropertyName] = WStringValue;
                    file.seekg(wif.tellg());
                    break;
                }
                case ESteamAppPropertyType::SAPT_Int32: {

                    file.read((char*)&int32Value, sizeof(int32Value));
                    obj[*pPropertyName] = int32Value;
                    break;
                }
                case ESteamAppPropertyType::SAPT_Float: {
                    file.read((char*)&floatValue, sizeof(floatValue));
                    obj[*pPropertyName] = floatValue;
                    break;
                }
                case ESteamAppPropertyType::SAPT_Color: {
                    nlohmann::json arr{ nlohmann::json::value_t::array };
                    file.read((char*)&uint8Value, sizeof(uint8Value));
                    arr.push_back(uint8Value);
                    file.read((char*)&uint8Value, sizeof(uint8Value));
                    arr.push_back(uint8Value);
                    file.read((char*)&uint8Value, sizeof(uint8Value));
                    arr.push_back(uint8Value);
                    obj[*pPropertyName] = arr;
                    break;
                }
                case ESteamAppPropertyType::SAPT_Uint64: {
                    file.read((char*)&uint64Value, sizeof(uint64Value));
                    obj[*pPropertyName] = uint64Value;
                    break;
                }
                }
            }
            };
        ReadPropertyTable(pSteamAppInfo->PropertyTable);
    }
    return pSteamCachedLibrary;
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