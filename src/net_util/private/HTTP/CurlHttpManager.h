#pragma once

#include <set>
#include <list>
#include <unordered_map>
#include <mutex>
#include <std_ext.h>
#include <concurrentqueue.h>
#include "HTTP/HttpManager.h"
#include "HTTP/CurlHttpRequest.h"
#include "net_export_defs.h"
typedef std::shared_ptr<class FCurlHttpRequest> CurlHttpRequestPtr;

typedef struct {
    HttpRequestPtr HttpReq;
    int64_t OldSize;
    int64_t NewSize;
    uint32_t RequestID;
}CurlDownloadProgress_t;
//typedef std::shared_ptr<class IHttpResponse> HttpResponsePtr;
class FCurlHttpManager :public IHttpManager {

public:
    enum ECurlState {
        INVALID,
        READY,
    };
    FCurlHttpManager();


    HttpRequestPtr NewRequest() override;
    void FreeRequest(HttpRequestPtr) override;
    bool ProcessRequest(HttpRequestPtr) override;
    void Tick(float delSec) override;
    void HttpThreadTick(float delSec) override;

    static struct CurlRequestOptions_t
    {
        CurlRequestOptions_t()
            : bVerifyPeer(false)
            , bUseHttpProxy(false)
            , bDontReuseConnections(false)
            , CertBundlePath(nullptr)
        {}

        /** Prints out the options to the log */
        void Log() {};

        /** Whether or not should verify peer certificate (disable to allow self-signed certs) */
        bool bVerifyPeer;

        /** Whether or not should use HTTP proxy */
        bool bUseHttpProxy;

        /** Forbid reuse connections (for debugging purposes, since normally it's faster to reuse) */
        bool bDontReuseConnections;

        /** Address of the HTTP proxy */
        std::string HttpProxyAddress;

        /** A path to certificate bundle */
        const char* CertBundlePath;
    }
    CurlRequestOptions;

    CURLSH* GetShareHandle() { return ShareHandle; }
private:



    size_t UploadCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq);
    size_t ReceiveResponseHeaderCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq);
    size_t ReceiveResponseBodyCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq);
    static size_t StaticUploadCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData);
    static size_t StaticReceiveResponseHeaderCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData);
    static size_t StaticReceiveResponseBodyCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData);
    void HttpThreadAddTask();
    bool SetupLocalRequest(CurlHttpRequestPtr);
    bool InitRequest(CurlHttpRequestPtr);
    bool SetupRequest(CurlHttpRequestPtr);
    void FinishRequest(CurlHttpRequestPtr creq);


    ECurlState CurlState{ INVALID };
    std::set<CurlHttpRequestPtr> WaitToFree;
    //mutil:wr
    std::atomic_uint32_t RequestIDCounter{ 0 };
    moodycamel::ConcurrentQueue<CurlHttpRequestPtr> FreeToUseRequests;

    //http:wr mutil:wr
    moodycamel::ConcurrentQueue<CurlHttpRequestPtr> RunningRequests;
    moodycamel::ConcurrentQueue<CurlHttpRequestPtr> FinishedRequests;
    moodycamel::ConcurrentQueue<CurlDownloadProgress_t> RunningProgressList;

    //http thread
    std::unordered_map<void*, CurlHttpRequestPtr, pointer_hash> HandlesToRequests;
    CURLM* MultiHandle{ nullptr };
    CURLSH* ShareHandle{ nullptr };

};