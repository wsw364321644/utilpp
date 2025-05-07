#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>
#include <raw_file.h>
#include <HTTP/HttpManager.h>
#include <concurrentqueue.h>
#include "Downloader/Downloader.h"

class FDownloadFile;
class FDownloadBuf;
struct file_chunk_t;
typedef std::list<std::shared_ptr<FDownloadBuf>>  BufList;

class FDownloader : public IDownloader
{
public:
    static FDownloader* Instance();
    ~FDownloader();
    DownloadTaskHandle_t AddTask(std::u8string_view url, std::vector<uint8_t>& contentBuf);
    DownloadTaskHandle_t AddTask(std::u8string_view url, const std::filesystem::path& folder);
   
    void RemoveTask(DownloadTaskHandle_t);
    bool RegisterDownloadProgressDelegate(DownloadTaskHandle_t, FDownloadProgressDelegate);
    bool RegisterDownloadFinishedDelegate(DownloadTaskHandle_t, FDownloadFinishedDelegate);
    bool RegisterGetFileInfoDelegate(DownloadTaskHandle_t, FGetFileInfoDelegate);
    std::shared_ptr<TaskStatus_t> GetTaskStatus(DownloadTaskHandle_t handle);
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

    // max size of BufPool
    inline static constexpr uint32_t BUF_NUM = 4;

    // max file_chunk_t num in FDownloadBuf
    inline static constexpr uint32_t BUF_CHUNK_NUM = 3;

    typedef std::unordered_map<CommonHandle_t, std::shared_ptr<FDownloadFile>> TaskContainer;
    TaskContainer Files;
    std::set<CommonHandle_t> RequireRemoveFiles;
    HttpManagerPtr pHttpManager;
    BufList BufPool;

    std::shared_ptr<TaskStatus_t> OutStatus;
    //trans data between io and main
    std::shared_ptr<FDownloadBuf> BufInIOBuf[BUF_NUM];
    moodycamel::ConcurrentQueue<std::shared_ptr<FDownloadBuf>> BufInIO;
    std::shared_ptr<FDownloadBuf> BufIOCompleteBuf[BUF_NUM];
    moodycamel::ConcurrentQueue<std::shared_ptr<FDownloadBuf>> BufIOComplete;
};