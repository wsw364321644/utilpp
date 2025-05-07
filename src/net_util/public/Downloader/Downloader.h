#pragma once
#include <string>
#include <vector>

#include "Downloader/DownloaderDef.h"
#include "net_export_defs.h"

class  IDownloader
{
public:
    SIMPLE_NET_EXPORT static IDownloader* Instance();
    virtual ~IDownloader() = default;;
    virtual DownloadTaskHandle_t AddTask(std::u8string_view url, std::vector<uint8_t>& contentBuf) = 0;
    virtual DownloadTaskHandle_t AddTask(std::u8string_view url, const std::filesystem::path& folder) = 0;

    virtual void RemoveTask(DownloadTaskHandle_t) = 0;
    virtual bool RegisterDownloadProgressDelegate(DownloadTaskHandle_t, FDownloadProgressDelegate) = 0;
    virtual bool RegisterDownloadFinishedDelegate(DownloadTaskHandle_t, FDownloadFinishedDelegate) = 0;
    virtual bool RegisterGetFileInfoDelegate(DownloadTaskHandle_t, FGetFileInfoDelegate) = 0;
    virtual std::shared_ptr<TaskStatus_t> GetTaskStatus(DownloadTaskHandle_t handle) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void NetThreadTick(float delSec) = 0;
    virtual void IOThreadTick(float delSec) = 0;

};