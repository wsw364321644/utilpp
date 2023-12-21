#pragma once
#include <handle.h>
#include <filesystem>
#include <vector>
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

typedef struct DownloadFileInfo
{
    std::filesystem::path FilePath;
    int64_t FileSize{ 0 };
    uint32_t ChunkNum{ 0 };
}DownloadFileInfo_t;

typedef struct TaskStatus_s {
    uint64_t DownloadSize{ 0 };
    uint64_t PreDownloadSize{ 0 };
    uint64_t LastTime{ 0 };
    uint64_t PreTime{ 0 };
    bool IsCompelete{ false };
    std::vector<std::byte> ChunksCompleteFlag;
}TaskStatus_t;

typedef std::function< void(DownloadTaskHandle, std::shared_ptr<DownloadFileInfo_t>)> FGetFileInfoDelegate;
typedef std::function< void(DownloadTaskHandle, std::shared_ptr <TaskStatus_t>)>  FDownloadProgressDelegate;
typedef std::function< void(DownloadTaskHandle, EDownloadCode)> FDownloadFinishedDelegate;

enum class EFileTaskStatus {
    Idle,
    Init,
    Download,
    Finished,
};