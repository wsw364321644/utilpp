#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>
#include <RawFile.h>
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
    friend struct FIterator;
    static FDownloader* Instance();
    ~FDownloader();
    FDownloadTaskIterator Begin() override;
    FDownloadTaskIterator End() override;
    DownloadTaskHandle_t AddTask(std::u8string_view url, FCharBuffer& contentBuf) override;
    DownloadTaskHandle_t AddTask(std::u8string_view url, const std::filesystem::path& folder) override;
    void LoadDiskTask(std::u8string_view) override;
    void RemoveTask(DownloadTaskHandle_t) override;
    bool RegisterDownloadProgressDelegate(DownloadTaskHandle_t, FDownloadProgressDelegate) override;
    bool RegisterDownloadFinishedDelegate(DownloadTaskHandle_t, FDownloadFinishedDelegate) override;
    bool RegisterGetFileInfoDelegate(DownloadTaskHandle_t, FGetFileInfoDelegate) override;
    std::shared_ptr<TaskStatus_t> GetTaskStatus(DownloadTaskHandle_t handle) override;
    std::shared_ptr<DownloadFileInfo> GetTaskInfo(DownloadTaskHandle_t handle) override;
    void Tick(float delSec)  override;
    void IOThreadTick(float delSec) override;
    FDownloader& operator=(const FDownloader& other) = delete;
    FDownloader(FDownloader&) = delete;
    FDownloader& operator=(const FDownloader&& other) = delete;
    FDownloader(FDownloader&&) = delete;

protected:
private:
    FDownloader();
    std::shared_ptr<TaskStatus_t> GetTaskStatus(std::shared_ptr<FDownloadFile> pfile);
    std::shared_ptr<DownloadFileInfo> GetTaskInfo(std::shared_ptr<FDownloadFile> pfile);
    DownloadTaskHandle_t AddTask(FDownloadFile* file);
    void TransferBuf();
    std::shared_ptr<FDownloadBuf> InsertInBuf(std::shared_ptr<file_chunk_t>);
    void StartDownloadWithoutContentLength(std::shared_ptr<FDownloadFile> pfile);

    uint32_t ParallelChunkPerFile{ 3 };
    uint32_t ParallelChunkToltal{ 5 };

    // max size of BufPool
    inline static constexpr uint32_t BUF_NUM = 4;

    // max file_chunk_t num in FDownloadBuf
    inline static constexpr uint32_t BUF_CHUNK_NUM = 3;

    typedef std::unordered_map<DownloadTaskHandle, std::shared_ptr<FDownloadFile>> TaskContainer;
    TaskContainer Files;
    std::set<DownloadTaskHandle> RequireRemoveFiles;
    HttpManagerPtr pHttpManager;
    BufList BufPool;

    std::shared_ptr<TaskStatus_t> OutStatus;
    std::shared_ptr<DownloadFileInfo> OutFileInfo;
    //trans data between io and main
    std::shared_ptr<FDownloadBuf> BufInIOBuf[BUF_NUM];
    moodycamel::ConcurrentQueue<std::shared_ptr<FDownloadBuf>> BufInIO;
    std::shared_ptr<FDownloadBuf> BufIOCompleteBuf[BUF_NUM];
    moodycamel::ConcurrentQueue<std::shared_ptr<FDownloadBuf>> BufIOComplete;
};


struct FIterator : public FDownloadTaskIterator::IIterator {
    FIterator(FDownloader* _Downloader) :Downloader(_Downloader) {}
    void next() override {
        if (Itr != Downloader->Files.end()) {
            ++Itr;
        }
    }
    const DownloadTaskHandle_t& deref() const override{
        return Itr->first;
    }
    std::unique_ptr<IIterator> clone() const override {
        auto out=std::make_unique<FIterator>(Downloader);
        out->Itr = Itr;
        return out;
    }
    virtual bool equal(IIterator* other) const override {
        auto r=dynamic_cast<FIterator*>(other);
        if (r&&r->Itr == Itr) {
            return true;
        }
        return false;
    }
    FDownloader::TaskContainer::iterator Itr;
    FDownloader* Downloader;
};