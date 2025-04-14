#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>
#include <raw_file.h>
#include <HTTP/HttpManager.h>
#include <ThroughCRTWrapper.h>
#include "Downloader/DownloaderDef.h"
#include "net_export_defs.h"
class FDownloadFile;
class FDownloadBuf;
typedef struct file_chunk_s file_chunk_t;
typedef std::list<std::shared_ptr<FDownloadBuf>>  BufList;
class SIMPLE_NET_EXPORT FDownloader
{
public:
    static FDownloader* Instance();
    ~FDownloader();
    //DownloadTaskHandle AddTask(std::string url, std::string folder);
    DownloadTaskHandle_t AddTask(ThroughCRTWrapper<std::string> url, std::string* content);
    DownloadTaskHandle_t AddTask(ThroughCRTWrapper<std::string>  url, ThroughCRTWrapper < std::filesystem::path> folder);
   
    void RemoveTask(DownloadTaskHandle_t);
    bool RegisterDownloadProgressDelegate(DownloadTaskHandle_t, FDownloadProgressDelegate);
    bool RegisterDownloadFinishedDelegate(DownloadTaskHandle_t, FDownloadFinishedDelegate);
    bool RegisterGetFileInfoDelegate(DownloadTaskHandle_t, FGetFileInfoDelegate);
    std::optional<TaskStatus_t> GetTaskStatus(DownloadTaskHandle_t handle);
    void Tick(float delSec);
    void NetThreadTick(float delSec);
    void IOThreadTick(float delSec);
    FDownloader& operator=(const FDownloader& other) = delete;
    FDownloader(FDownloader&) = delete;
    FDownloader& operator=(const FDownloader&& other) = delete;
    FDownloader(FDownloader&&) = delete;

protected:
private:
    FDownloader();
    DownloadTaskHandle_t AddTask(FDownloadFile* file);
    void TransferBuf();
    std::shared_ptr<FDownloadBuf> InsertInBuf(std::shared_ptr<file_chunk_t>);


    uint32_t ParallelChunkPerFile{ 3 };
    uint32_t ParallelChunkToltal{ 5 };

    static const uint32_t BUF_NUM;
    static const uint32_t BUF_CHUNK_NUM;

    std::mutex BufInIOMtx;
    std::mutex BufIOCompleteMtx;

    typedef std::unordered_map<CommonHandle_t, std::shared_ptr<FDownloadFile>> TaskContainer;
    TaskContainer Files;
    std::set<CommonHandle_t> RequireRemoveFiles;
    HttpManagerPtr pHttpManager;
    BufList BufPool;

    //trans data between io and main
    BufList  BufInIO;
    BufList  BufIOComplete;
};