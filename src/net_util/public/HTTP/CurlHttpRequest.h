#pragma once
#include "IHttpRequest.h"
#include <curl/curl.h>
#include <unordered_map>
#include <list>


class FCurlHttpManager;
class FCurlHttpResponse;

class FCurlHttpRequest :public IHttpRequest {

public:
    virtual ~FCurlHttpRequest();
    virtual std::string GetURL() override;
    virtual std::string GetURLParameter(const std::string& ParameterName) override;
    virtual std::string GetHeader(const std::string& HeaderName) override;
    virtual std::vector<std::string> GetAllHeaders() override;
    virtual std::string GetContentType() override;
    virtual int64_t GetContentLength() override;
    virtual const std::vector<uint8_t>& GetContent() override;
    virtual std::vector<MimePart_t> GetAllMime() override;

    virtual std::string GetVerb() override;
    virtual void SetVerb(const std::string& Verb) override;
    virtual void SetURL(const std::string& URL) override;
    virtual void SetHost(const std::string& Host) override;
    virtual void SetPath(const std::string& Path) override;
    virtual void SetScheme(const std::string& Scheme) override;
    virtual void SetPortNum(uint32_t Port) override;
    virtual void SetQuery(const std::string& QueryName, const std::string& QueryValue) override;
    virtual void SetContent(const std::vector<uint8_t>& ContentPayload) override;
    virtual void SetContentAsString(const std::string& ContentString) override;

    virtual void SetHeader(const std::string& HeaderName, const std::string& HeaderValue) override;
    virtual void AppendToHeader(const std::string& HeaderName, const std::string& AdditionalHeaderValue) override;
    virtual void SetMimePart(const MimePart_t part)override;
    virtual void SetRange(uint64_t begin, uint64_t end)override;
    virtual bool ProcessRequest() override;
    virtual HttpRequestCompleteDelegateType& OnProcessRequestComplete() override;
    virtual HttpRequestProgressDelegateType& OnRequestProgress() override;
    virtual void CancelRequest() override;
    virtual EHttpRequestStatus GetStatus() override;
    virtual const HttpResponsePtr GetResponse() const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float GetElapsedTime() override;
    friend class FCurlHttpManager;
private:
    FCurlHttpRequest(FCurlHttpManager* inManager);
    size_t DebugCallback(CURL* Handle, curl_infotype DebugInfoType, char* DebugInfo, size_t DebugInfoSize, void* UserData);
    static size_t StaticDebugCallback(CURL* Handle, curl_infotype DebugInfoType, char* DebugInfo, size_t DebugInfoSize, void* UserData);

    /** Pointer to an easy handle specific to this request */
    CURL* EasyHandle{ 0 };
    /** List of custom headers to be passed to CURL */
    curl_slist* HeaderList{ 0 };
    curl_mime* Mime{ 0 };
    int32_t BytesSent{ 0 };
    /** Elapsed time since the last received HTTP response. */
    float TimeSinceLastResponse{ 0 };
    CURLMcode CurlAddToMultiResult;
    CURLcode CurlCompletionResult;
    bool bCompleted{ false };
    bool bCanceled{ false };
    std::list<std::pair<uint64_t, uint64_t>> Ranges;

    FCurlHttpManager* Manager;
    std::shared_ptr<FCurlHttpResponse> Response;
    EHttpRequestStatus CompletionStatus{ EHttpRequestStatus::NotStarted };
    uint32_t Port{ std::numeric_limits<uint32_t>::max() };
    std::string Verb;
    std::string URL;
    std::string Host;
    std::string Path;
    std::string Scheme;
    std::unordered_map<std::string, std::string> Queries;
    std::unordered_map<std::string, std::string> Headers;
    std::vector<MimePart_t> MimeParts;
    std::vector<uint8_t> Content;

    HttpRequestCompleteDelegateType HttpRequestCompleteDelegate;
    HttpRequestProgressDelegateType HttpRequestProgressDelegate;
};


class FCurlHttpResponse :public IHttpResponse {

public:

    FCurlHttpResponse(FCurlHttpRequest* inCurlRequest) :CurlRequest(inCurlRequest) {};
    virtual ~FCurlHttpResponse() {};
    virtual std::string GetURL()override { return ""; };
    virtual std::string GetURLParameter(const std::string& ParameterName)override { return ""; };
    virtual std::string GetHeader(const std::string& HeaderName)override;
    virtual std::vector<std::string> GetAllHeaders()override;
    virtual std::vector<MimePart_t> GetAllMime() override;
    virtual std::string GetContentType();
    virtual int64_t GetContentLength() { return ContentLength; };
    virtual const std::vector<uint8_t>& GetContent();
    FCurlHttpRequest* GetRequest() { return CurlRequest; }
    virtual void SetContentBuf(void* Ptr, int64_t Len) override;
    virtual int32_t GetResponseCode() { return HttpCode; };
    virtual int64_t GetContentBytesRead() { return TotalBytesRead; };
    virtual std::string GetContentAsString();

    void ContentAppend(char* Data, size_t Len);
    friend class FCurlHttpManager;
private:
    FCurlHttpRequest* CurlRequest;
    bool bSucceeded{ false };
    bool bIsReady{ false };
    int32_t HttpCode{ 0 };
    std::atomic_int64_t ContentLength{ -1 };
    std::atomic_int64_t TotalBytesRead{ 0 };
    std::unordered_map<std::string, std::string> Headers;
    std::vector<uint8_t> Content;
    char* UserBuf{ 0 };
    int64_t UserBufLen{ 0 };
};