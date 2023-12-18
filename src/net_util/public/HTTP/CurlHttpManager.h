#pragma once
#include "HttpManager.h"
#include "CurlHttpRequest.h"
#include <set>
#include <list>
#include <unordered_map>
#include <mutex>

typedef std::shared_ptr<class FCurlHttpRequest> CurlHttpRequestPtr;
typedef struct {
    FCurlHttpRequest* HttpReq;
    int64_t OldSize;
    int64_t NewSize;
}CurlDownloadProgress_t;
//typedef std::shared_ptr<class IHttpResponse> HttpResponsePtr;
class FCurlHttpManager :public FHttpManager {

public:
    enum ECurlState {
        INVALID,
        READY,
    };
    FCurlHttpManager();


    virtual HttpRequestPtr NewRequest() override;
    virtual bool ProcessRequest(HttpRequestPtr);
    virtual void Tick() ;

    void HttpThreadTick();

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



    size_t UploadCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes,FCurlHttpRequest* creq);
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
    std::unordered_map<void*, CurlHttpRequestPtr> HandlesToRequests;
    ECurlState CurlState{ INVALID };

    //http:wr main:wr
    std::mutex ReqMutex;
    std::set<CurlHttpRequestPtr> FinishedRequests;
    std::list<CurlHttpRequestPtr> RunningRequests;
    std::mutex ProgressMutex;
    std::list<CurlDownloadProgress_t> RunningProgressList;

    //http thread
    std::list<CurlHttpRequestPtr> RunningThreadedRequests;
    CURLM* MultiHandle{nullptr};
    CURLSH* ShareHandle{ nullptr };

};