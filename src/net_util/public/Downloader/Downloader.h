#pragma once

#include <CharBuffer.h>
#include "Downloader/DownloaderDef.h"
#include "net_export_defs.h"

class FDownloadTaskIterator {
public:

    using value_type = uint32_t;
    using pointer = DownloadTaskHandle_t*;
    using reference = DownloadTaskHandle_t&;

    struct IIterator {
        virtual ~IIterator()=default;
        virtual void next() = 0;
        virtual const DownloadTaskHandle_t& deref() const = 0;
        virtual std::unique_ptr<IIterator> clone() const = 0;
        virtual bool equal(IIterator* other) const = 0;
    };
    FDownloadTaskIterator() =default;
    FDownloadTaskIterator(FDownloadTaskIterator&& r) {
        itr = std::move(r.itr);
    }
    FDownloadTaskIterator(FDownloadTaskIterator& r) {
        itr = r.itr->clone();
    }
    const DownloadTaskHandle_t& operator*() const {
        return itr->deref();
    }
    const DownloadTaskHandle_t* operator->() const {
        return &itr->deref();
    }
    FDownloadTaskIterator& operator++() {
        itr->next();
    }
    FDownloadTaskIterator operator++(int) {
        FDownloadTaskIterator out(*this);
        itr->next();
        return out;
    }

    friend bool operator==(const FDownloadTaskIterator& a, const FDownloadTaskIterator& b) {
        return a.itr->equal(b.itr.get());
    }

    friend bool operator!=(const FDownloadTaskIterator& a, const FDownloadTaskIterator& b) {
        return !a.itr->equal(b.itr.get());
    }
    std::unique_ptr<IIterator> itr;
};

/**
* rely on IHttpManager
* need call Tick&HttpThreadTick on IHttpManager external
*/
class  IDownloader
{
public:



    SIMPLE_NET_EXPORT static IDownloader* Instance();
    virtual ~IDownloader() = default;
    virtual FDownloadTaskIterator Begin() = 0;
    virtual FDownloadTaskIterator End() = 0;
    virtual DownloadTaskHandle_t AddTask(std::u8string_view url, FCharBuffer& contentBuf) = 0;
    virtual DownloadTaskHandle_t AddTask(std::u8string_view url, const std::filesystem::path& folder) = 0;
    virtual void LoadDiskTask(std::u8string_view) = 0;

    virtual void RemoveTask(DownloadTaskHandle_t) = 0;
    virtual bool RegisterDownloadProgressDelegate(DownloadTaskHandle_t, FDownloadProgressDelegate) = 0;
    virtual bool RegisterDownloadFinishedDelegate(DownloadTaskHandle_t, FDownloadFinishedDelegate) = 0;
    virtual bool RegisterGetFileInfoDelegate(DownloadTaskHandle_t, FGetFileInfoDelegate) = 0;
    virtual std::shared_ptr<TaskStatus_t> GetTaskStatus(DownloadTaskHandle_t handle) = 0;
    virtual std::shared_ptr<DownloadFileInfo> GetTaskInfo(DownloadTaskHandle_t handle) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void IOThreadTick(float delSec) = 0;

};