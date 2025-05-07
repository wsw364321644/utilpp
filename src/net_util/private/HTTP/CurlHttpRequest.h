#pragma once
#include "HTTP/IHttpRequest.h"

#include <unordered_map>
#include <list>
#include <curl/curl.h>

class FCurlHttpManager;
class FCurlHttpResponse;

class FCurlHttpRequest :public IHttpRequest {

public:
    ~FCurlHttpRequest() override;
    std::string_view GetURL() override;
    std::string_view GetURLParameter(const std::string_view ParameterName) override;
    std::string_view GetHeader(const std::string_view HeaderName) override;
    std::vector<std::string> GetAllHeaders() override;
    std::string_view GetContentType() override;
    int64_t GetContentLength() override;
    const std::vector<uint8_t>& GetContent() override;
    std::vector<MimePart_t> GetAllMime() override;

    std::string_view GetVerb() override;
    void SetVerb(const std::string_view Verb) override;
    void SetURL(const std::string_view URL) override;
    void SetHost(const std::string_view) override;
    void SetPath(const std::string_view Path) override;
    void SetScheme(const std::string_view Scheme) override;
    void SetPortNum(uint32_t Port) override;
    void SetQuery(const std::string_view QueryName, const std::string_view QueryValue) override;
    void SetContent(const std::vector<uint8_t>& ContentPayload) override;
    void SetContentAsString(const std::string_view ContentString) override;

    void SetHeader(const std::string_view HeaderName, const std::string_view HeaderValue) override;
    void AppendToHeader(const std::string_view HeaderName, const std::string_view AdditionalHeaderValue) override;
    void SetMimePart(InMimePart_t part)override;
    void SetRange(uint64_t begin, uint64_t end)override;
    bool ProcessRequest() override;
    void Clear();
    HttpRequestCompleteDelegateType& OnProcessRequestComplete() override;
    HttpRequestProgressDelegateType& OnRequestProgress() override;
    void CancelRequest() override;
    EHttpRequestStatus GetStatus() override;
    const HttpResponsePtr GetResponse() const override;
    void Tick(float DeltaSeconds) override;
    float GetElapsedTime() override;
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

    uint32_t RequestID{ 0 };
    HttpRequestCompleteDelegateType HttpRequestCompleteDelegate;
    HttpRequestProgressDelegateType HttpRequestProgressDelegate;
};


class FCurlHttpResponse :public IHttpResponse {

public:

    FCurlHttpResponse(FCurlHttpRequest* inCurlRequest) :CurlRequest(inCurlRequest) {};
    ~FCurlHttpResponse() override {};
    std::string_view GetURL()override { return EffectiveUrl; };
    std::string_view GetURLParameter(const std::string_view ParameterName)override { return ""; }
    std::string_view GetHeader(const std::string_view HeaderName)override;
    std::vector<std::string> GetAllHeaders()override;
    std::vector<MimePart_t> GetAllMime() override;
    std::string_view GetContentType() override;
    int64_t GetContentLength() override { return ContentLength; }
    const std::vector<uint8_t>& GetContent() override;
    FCurlHttpRequest* GetRequest() { return CurlRequest; }
    void SetContentBuf(void* Ptr, int64_t Len) override;
    int32_t GetResponseCode() override { return HttpCode; }
    int64_t GetContentBytesRead() override { return TotalBytesRead; }
    std::u8string_view GetContentAsString() override;

    void ContentAppend(char* Data, size_t Len);
    void Clear();
    friend class FCurlHttpManager;
private:
    FCurlHttpRequest* CurlRequest;
    bool bSucceeded{ false };
    bool bIsReady{ false };
    int32_t HttpCode{ 0 };
    std::string EffectiveUrl{};
    std::atomic_int64_t ContentLength{ -1 };
    std::atomic_int64_t TotalBytesRead{ 0 };
    std::unordered_map<std::string, std::string> Headers;
    std::vector<uint8_t> Content;
    char* UserBuf{ 0 };
    int64_t UserBufLen{ 0 };
};