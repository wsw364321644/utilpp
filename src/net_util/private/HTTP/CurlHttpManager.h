#pragma once

#include <set>
#include <list>
#include <unordered_map>
#include <mutex>
#include <std_ext.h>
#include <tbb/concurrent_queue.h>
#include "HTTP/HttpManager.h"
#include "HTTP/CurlHttpRequest.h"
#include "net_export_defs.h"
typedef std::shared_ptr<class FCurlHttpRequest> CurlHttpRequestPtr;
typedef struct {
    FCurlHttpRequest* HttpReq;
    int64_t OldSize;
    int64_t NewSize;
}CurlDownloadProgress_t;
//typedef std::shared_ptr<class IHttpResponse> HttpResponsePtr;
class SIMPLE_NET_EXPORT FCurlHttpManager :public IHttpManager {

public:
    enum ECurlState {
        INVALID,
        READY,
    };
    FCurlHttpManager();


    HttpRequestPtr NewRequest() override;
    bool ProcessRequest(HttpRequestPtr) override;
    void Tick(float delSec) override;

    void HttpThreadTick(float delSec);

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

    //std::unordered_map<CurlHttpRequestPtr, CurlHttpRequestPtr> ReqsMap;
    std::list<CurlHttpRequestPtr> Reqs;
    std::unordered_map<void*, CurlHttpRequestPtr, pointer_hash> HandlesToRequests;
    ECurlState CurlState{ INVALID };

    //http:wr main:wr
    tbb::concurrent_queue<CurlHttpRequestPtr> RunningRequests;
    tbb::concurrent_queue<CurlHttpRequestPtr> FinishedRequests;
    tbb::concurrent_queue<CurlDownloadProgress_t> RunningProgressList;

    //http thread
    std::list<CurlHttpRequestPtr> RunningThreadedRequests;
    CURLM* MultiHandle{ nullptr };
    CURLSH* ShareHandle{ nullptr };

};