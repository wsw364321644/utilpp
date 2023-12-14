#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <filesystem>
#include <optional>
#include <handle.h>
#include <raw_file.h>
#include <HTTP/CurlHttpManager.h>
typedef struct DownloadTaskHandle : CommonHandle_t
{
    DownloadTaskHandle() :CommonHandle_t() {}
    DownloadTaskHandle(CommonHandle_t h) :CommonHandle_t(h) {}
    static std::atomic_uint32_t task_count;
}DownloadTaskHandle_t;

enum class EDownloadCode {
    OK,
    PARAMS_ERROR,
    INTERNAL_ERROR,
    FINISHED,
    COULDNT_CONNECT,
    TIMEOUT,
    SSL_CONNECT_ERROR,
    SERVER_ERROR
};


typedef std::function< void(int64_t total, int64_t now)>  FDownloadProgressDelegate;
typedef std::function< void(EDownloadCode code, int http_code)> FDownloadFinishedDelegate;
class FDownloadFile;
class FDownloadBuf;
typedef struct file_chunk_s {
    uint32_t ChunkIndex{ 0 };
    uint32_t DownloadSize{ 0 };
    std::shared_ptr<FDownloadFile> File;
    std::shared_ptr<FDownloadBuf> Buf;
    char* BufCursor{ nullptr };
    std::pair<uint64_t, uint64_t> GetRange() {
        auto begin = FDownloadFile::CHUNK_SIZE * ChunkIndex;
        auto end = begin + FDownloadFile::CHUNK_SIZE - 1;
        return std::pair(begin, end + 1 > File->Size ? File->Size : end + 1);
    }
    uint64_t GetChunkSize() {
        return FDownloadFile::CHUNK_SIZE;
    }
}file_chunk_t;

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

enum class EFileTaskStatus {
    Idle,
    Init,
    Download,
    Finished,
};

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

    //FDownloadFile& operator=(const FDownloadFile& other) = delete;
    //FDownloadFile(FDownloadFile&) = delete;
    //FDownloadFile& operator=(const FDownloadFile&& other) = delete;
    //FDownloadFile(FDownloadFile&&) = delete;

    static const uint32_t CHUNK_SIZE;

    EDownloadCode Code{ EDownloadCode::OK };
    std::string URL;
    EFileTaskStatus Status{ EFileTaskStatus::Idle };
    uint64_t Size{ 0 };
    std::filesystem::path Path;
    std::string* Content{ nullptr };
    std::vector<std::byte> ChunksCompleteFlag;
    std::vector<std::byte> ChunksDownloadFlag;
    uint32_t ChunkNum;
    std::set<HttpRequestPtr> RequestPool;
    std::unordered_map<HttpRequestPtr, file_chunk_t> Requests;
    bool Open();


    CRawFile FileStream;
private:
    FDownloadFile();
};

class FDownloadFileNet :public FDownloadFile {
    FDownloadFileNet(FDownloadFile& right) :FDownloadFile(right) {}
};
class FDownloadFileLocal :public FDownloadFile {
    FDownloadFileLocal(FDownloadFile& right) :FDownloadFile(right) {}
public:
    uint64_t DownloadSize;
    std::shared_ptr<FDownloadFileNet> NetFileTask;
};
class FDownloader
{
public:
    static FDownloader* Instance();
    ~FDownloader();
    DownloadTaskHandle AddTask(std::string url, std::string folder);
    DownloadTaskHandle AddTask(std::string url, std::string* content);
    DownloadTaskHandle AddTask(std::string url, std::filesystem::path folder);

    void Tick();
    void NetThreadTick();
    void IOThreadTick();
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
    std::mutex FilesMtx;
    //std::mutex FilesInProgressMtx;
    std::mutex BufInIOMtx;
    std::mutex BufIOCompleteMtx;
    std::mutex BufPoolMtx;

    std::unordered_map<CommonHandle_t, std::shared_ptr<FDownloadFileLocal>> Files;
    FCurlHttpManager HttpManager;

    //std::unordered_map<CommonHandle_t, std::shared_ptr<FDownloadFileNet>> FilesInProgress;
    BufList BufPool;
    BufList  BufInIO;
    BufList  BufIOComplete;
};