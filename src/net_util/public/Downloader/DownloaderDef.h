#pragma once
#include <handle.h>
#include <filesystem>
#include <vector>
#include "net_export_defs.h"

typedef struct SIMPLE_NET_EXPORT DownloadTaskHandle : CommonHandle32_t
{
    DownloadTaskHandle() :CommonHandle32_t() {}
    DownloadTaskHandle(const CommonHandle32_t h) :CommonHandle32_t(h) {}
    static std::atomic_uint32_t task_count;
}DownloadTaskHandle_t ;

namespace std
{
    template<>
    struct equal_to<DownloadTaskHandle>
    {
        using argument_type = DownloadTaskHandle;
        using result_type = bool;
        constexpr bool operator()(const argument_type& lhs, const argument_type& rhs) const
        {
            return lhs.ID == rhs.ID;
        }
    };

    template<>
    class hash<DownloadTaskHandle>
    {
    public:
        size_t operator()(const DownloadTaskHandle& handle) const
        {
            return handle.ID;
        }
    };

    template<>
    struct less<DownloadTaskHandle>
    {
    public:
        size_t operator()(const DownloadTaskHandle& _Left, const DownloadTaskHandle& _Right) const
        {
            return _Left.operator<(_Right);
        }
    };
}

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
    std::vector<uint8_t> ChunksCompleteFlag;
}TaskStatus_t;

typedef std::function< void(const DownloadTaskHandle_t, std::shared_ptr<DownloadFileInfo_t>)> FGetFileInfoDelegate;
typedef std::function< void(const DownloadTaskHandle_t, std::shared_ptr <TaskStatus_t>)>  FDownloadProgressDelegate;
typedef std::function< void(const DownloadTaskHandle_t, EDownloadCode)> FDownloadFinishedDelegate;

enum class EFileTaskStatus :uint8_t{
    Idle,
    Init,
    DownloadChunk,
    Download,
    Finished,
};