#pragma once
#include <string>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>
#include <raw_file.h>
#include <HTTP/CurlHttpManager.h>
#include "Downloader/DownloaderDef.h"

class FDownloadFile;
class FDownloadBuf;
typedef struct file_chunk_s file_chunk_t;


class  FDownloadBuf : public std::enable_shared_from_this<FDownloadBuf> {
public:

    FDownloadBuf(uint32_t length) {
        Len = length;
        Buf = (char*)malloc(Len);
    }
    ~FDownloadBuf() {
        if (Buf) {
            free(Buf);
        }
    }
    bool InsertInBuf(std::shared_ptr < file_chunk_t> pchunk);
    bool RemoveChunk(std::shared_ptr < file_chunk_t> pchunk);

    char* Buf{ nullptr };
    uint32_t Len;
    std::list<std::shared_ptr<file_chunk_t>> ChunkList;
};

typedef std::list<std::shared_ptr<FDownloadBuf>>  BufList;

class FDownloadFile : public std::enable_shared_from_this<FDownloadFile> {
public:

    FDownloadFile(std::string url, std::string folder);
    FDownloadFile(std::string url, std::string* content);
    FDownloadFile(std::string url, std::filesystem::path folder);

    ~FDownloadFile();
    void SetFileSize(uint64_t size);
    std::shared_ptr<file_chunk_t> GetNotDownloadFilechunk();
    void RevertDownloadFilechunk(uint32_t ChunkIndex);
    bool IsAllChunkInDownload();
    uint64_t GetDownloadSize();
    bool IsIntact();

    size_t SavaDate(std::shared_ptr<file_chunk_t> file_chunk);
    bool CompleteChunk(std::shared_ptr<file_chunk_t> file_chunk);
    uint64_t GetChunkSize(uint32_t index);
    void CloseFileStream();
    void Finish(EDownloadCode err, std::string&& msg) {
        Finish(err);
        ErrorMsg = std::forward<std::string>(msg);
    }
    void Finish(EDownloadCode err) {
        Status = EFileTaskStatus::Finished;
        Code = err;
        ErrorMsg.clear();
    }

    //FDownloadFile& operator=(const FDownloadFile& other) = delete;
    //FDownloadFile(FDownloadFile&) = delete;
    //FDownloadFile& operator=(const FDownloadFile&& other) = delete;
    //FDownloadFile(FDownloadFile&&) = delete;

    static const uint32_t CHUNK_SIZE;
    EFileTaskStatus Status{ EFileTaskStatus::Idle };
    EDownloadCode Code{ EDownloadCode::OK };
    std::string ErrorMsg;
    std::string URL;
    std::atomic_int64_t Size{ 0 };
    std::atomic_uint64_t DownloadSize{ 0 };
    std::atomic_uint64_t PreDownloadSize{ 0 };
    std::atomic_uint64_t LastTime;
    std::atomic_uint64_t PreTime;
    std::filesystem::path Path;
    std::string* Content{ nullptr };
    std::vector<std::byte> ChunksCompleteFlag;
    std::vector<std::byte> ChunksDownloadFlag;
    uint32_t ChunkNum{ 0 };
    std::set<HttpRequestPtr> RequestPool;
    std::unordered_map<HttpRequestPtr, file_chunk_t> Requests;

    FDownloadProgressDelegate DownloadProgressDelegate;
    FDownloadFinishedDelegate DownloadFinishedDelegate;
    FGetFileInfoDelegate GetFileInfoDelegate;
private:
    bool Open();
    CRawFile FileStream;
    FDownloadFile();
};

typedef struct file_chunk_s {
    uint32_t ChunkIndex{ 0 };
    uint32_t DownloadSize{ 0 };
    std::shared_ptr<FDownloadFile> File;
    std::shared_ptr<FDownloadBuf> Buf;
    char* BufCursor{ nullptr };
    std::pair<uint64_t, uint64_t> GetRange() {
        auto begin = FDownloadFile::CHUNK_SIZE * ChunkIndex;
        auto end = begin + FDownloadFile::CHUNK_SIZE - 1;
        auto filesize = File->Size.load();
        return std::pair(begin, end > filesize - 1 ? filesize - 1 : end + 1);
    }
    uint64_t GetChunkSize() {
        return GetRange().second - GetRange().first + 1;
    }
}file_chunk_t;
//class FDownloadFileNet :public FDownloadFile {
//    FDownloadFileNet(FDownloadFile& right) :FDownloadFile(right) {}
//};
//class FDownloadFileLocal :public FDownloadFile {
//    FDownloadFileLocal(FDownloadFile& right) :FDownloadFile(right) {}
//public:
//    //std::shared_ptr<FDownloadFileNet> NetFileTask;
//};
class FDownloader
{
public:
    static FDownloader* Instance();
    ~FDownloader();
    //DownloadTaskHandle AddTask(std::string url, std::string folder);
    DownloadTaskHandle AddTask(std::string url, std::string* content);
    DownloadTaskHandle AddTask(std::string url, std::filesystem::path folder);
   
    void RemoveTask(DownloadTaskHandle);
    bool RegisterDownloadProgressDelegate(DownloadTaskHandle, FDownloadProgressDelegate);
    bool RegisterDownloadFinishedDelegate(DownloadTaskHandle, FDownloadFinishedDelegate);
    bool RegisterGetFileInfoDelegate(DownloadTaskHandle, FGetFileInfoDelegate);
    std::optional<TaskStatus_t> GetTaskStatus(DownloadTaskHandle handle);
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
    DownloadTaskHandle AddTask(FDownloadFile* file);
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
    FCurlHttpManager HttpManager;
    BufList BufPool;

    //trans data between io and main
    BufList  BufInIO;
    BufList  BufIOComplete;
};