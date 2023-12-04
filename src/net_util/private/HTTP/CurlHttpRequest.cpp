#include "HTTP/CurlHttpRequest.h"
#include "HTTP/CurlHttpManager.h"
#include <algorithm>
#include <iterator>
#include <uri.h>
#include <string_view>
#include <ranges>
#include <logger.h>


template <typename Out>
void split(const std::string& str,const char* delimstr, Out result) {
    //using std::operator""sv;
    //constexpr auto delim{ "&"sv};
    //auto delim = std::string_view{ delimstr, strlen(delimstr) };
    //for (const auto word : std::views::split(s, delimstr)) {
        //*result++ = std::string(std::string_view(word));
        //std::string(std::string_view(word));
    //}


    size_t pos = 0,cursor=0;
    std::string token;
    while ((pos = str.find(delimstr, cursor)) != std::string::npos) {
        token = str.substr(cursor, pos);
        *result++ = token;
        cursor = pos + strlen(delimstr);
    }
}



FCurlHttpRequest::FCurlHttpRequest(FCurlHttpManager* inManager) : Manager(inManager)
{
    
}

FCurlHttpRequest::~FCurlHttpRequest()
{

}

std::string FCurlHttpRequest::GetURL()
{
    return URL;
}

std::string FCurlHttpRequest::GetURLParameter(const std::string& ParameterName)
{
    ParsedURL_t res;
    ParseUrl(URL,&res);
    if (res.outQuery) {
        std::vector<std::string> pairs;
        split(*res.outQuery, "&", std::back_inserter(pairs));
        for (auto pair : pairs) {
            std::vector<std::string> kv;
            split(pair, "=", std::back_inserter(kv));
            if (kv[0] == ParameterName) {
                return kv[1];
            }
        }
    }
    return std::string();
}

std::string FCurlHttpRequest::GetHeader(const std::string& HeaderName)
{
    auto itr=Headers.find(HeaderName);
    if (itr != Headers.end()) {
        return itr->second;
    }
    return std::string();
}

std::vector<std::string> FCurlHttpRequest::GetAllHeaders()
{
    std::vector<std::string> out;
    std::transform(Headers.begin(), Headers.end(), std::back_inserter(out), [](auto& kv) { return kv.first+":"+ kv.second; });
    return out;
}

std::string FCurlHttpRequest::GetContentType()
{
    return GetHeader("Content-Type");
}


int32_t FCurlHttpRequest::GetContentLength()
{
    return Content.size();
}

const std::vector<uint8_t>& FCurlHttpRequest::GetContent()
{
    return Content;
}

std::vector<MimePart_t> FCurlHttpRequest::GetAllMime()
{
    return MimeParts;
}

std::string FCurlHttpRequest::GetVerb()
{
    return Verb;
}

void FCurlHttpRequest::SetVerb(const std::string& _Verb)
{
    Verb = _Verb;
}

void FCurlHttpRequest::SetURL(const std::string& _URL)
{
    URL = _URL;
}

void FCurlHttpRequest::SetHost(const std::string& _Host)
{
    Host = _Host;
}

void FCurlHttpRequest::SetPath(const std::string& _Path)
{
    Path = _Path;
}

void FCurlHttpRequest::SetScheme(const std::string& _Scheme)
{
    Scheme = _Scheme;
}

void FCurlHttpRequest::SetPortNum(uint32_t _Port)
{
    Port = _Port;
}

void FCurlHttpRequest::SetQuery(const std::string& QueryName, const std::string& QueryValue)
{
    Queries[QueryName] = QueryValue;
}

void FCurlHttpRequest::SetContent(const std::vector<uint8_t>& ContentPayload)
{
    Content = ContentPayload;
}

void FCurlHttpRequest::SetContentAsString(const std::string& ContentString)
{
    Content.assign(ContentString.begin(), ContentString.end());
}

void FCurlHttpRequest::SetHeader(const std::string& HeaderName, const std::string& HeaderValue)
{
    Headers[HeaderName] = HeaderValue;
}

void FCurlHttpRequest::AppendToHeader(const std::string& HeaderName, const std::string& AdditionalHeaderValue)
{
    auto itr = Headers.find(HeaderName);
    if (itr == Headers.end()) {
        SetHeader(HeaderName, AdditionalHeaderValue);
    }
    else {
        itr->second.append(",");
        itr->second.append(AdditionalHeaderValue);
    }
}

void FCurlHttpRequest::SetMimePart(const MimePart_t part)
{
    MimeParts.push_back(part);
}

bool FCurlHttpRequest::ProcessRequest()
{
    return Manager->ProcessRequest(shared_from_this());
}

HttpRequestCompleteDelegateType& FCurlHttpRequest::OnProcessRequestComplete()
{
    return HttpRequestCompleteDelegate;
}

HttpRequestProgressDelegateType& FCurlHttpRequest::OnRequestProgress()
{
    return HttpRequestProgressDelegate;
}

void FCurlHttpRequest::CancelRequest()
{
}

EHttpRequestStatus FCurlHttpRequest::GetStatus()
{
    return CompletionStatus;
}

const HttpResponsePtr FCurlHttpRequest::GetResponse() const
{
    return Response;
}

void FCurlHttpRequest::Tick(float DeltaSeconds)
{
}

float FCurlHttpRequest::GetElapsedTime()
{
    return 0.0f;
}

size_t FCurlHttpRequest::DebugCallback(CURL* Handle, curl_infotype DebugInfoType, char* DebugInfo, size_t DebugInfoSize, void* UserData)
{
    switch (DebugInfoType)
    {
    case CURLINFO_TEXT:
    {
        LOG_DEBUG("{:x}:{}", (int32_t)this, std::string(DebugInfo, DebugInfoSize));
    }
    break;

    case CURLINFO_HEADER_IN:
    {
        LOG_DEBUG("{:x}: Received header ({} bytes)", (int32_t)this, DebugInfoSize);
    }
    break;
    case CURLINFO_HEADER_OUT:
    {
        LOG_DEBUG("{:x}: Sent header ({} bytes) - {}", (int32_t)this, DebugInfoSize, std::string(DebugInfo, DebugInfoSize));
    }
    break;

    case CURLINFO_DATA_IN:
    {
        LOG_DEBUG("{:x}: Received data ({} bytes)", (int32_t)this, DebugInfoSize);
    }
    break;
    case CURLINFO_DATA_OUT:
    {
        LOG_DEBUG("{:x}: Sent data ({} bytes)", (int32_t)this, DebugInfoSize);
    }
    break;

    case CURLINFO_SSL_DATA_IN:
    {
        LOG_DEBUG("{:x}: Received SSL data ({} bytes)", (int32_t)this, DebugInfoSize);
    }
    break;
    case CURLINFO_SSL_DATA_OUT:
    {
        LOG_DEBUG("{:x}: Sent SSL data ({} bytes)", (int32_t)this, DebugInfoSize);
    }
    break;
    default:
    {
        LOG_DEBUG("{:x}: DebugCallback: Unknown DebugInfoType={} (DebugInfoSize: {} bytes)", (int32_t)this, (int32_t)DebugInfoType, DebugInfoSize);
    }
    break;
    }

    return 0;
}

size_t FCurlHttpRequest::StaticDebugCallback(CURL* Handle, curl_infotype DebugInfoType, char* DebugInfo, size_t DebugInfoSize, void* UserData)
{
    auto creq = (FCurlHttpRequest*)UserData;
    return creq->DebugCallback(Handle, DebugInfoType, DebugInfo, DebugInfoSize,UserData);
}


std::string FCurlHttpResponse::GetHeader(const std::string& HeaderName)
{
    auto itr = Headers.find(HeaderName);
    if (itr != Headers.end()) {
        return itr->second;
    }
    return std::string();
}

std::vector<std::string> FCurlHttpResponse::GetAllHeaders()
{
    std::vector<std::string> out;
    std::transform(Headers.begin(), Headers.end(), std::back_inserter(out), [](auto& kv) { return kv.first + ";" + kv.second; });
    return out;
}

std::vector<MimePart_t> FCurlHttpResponse::GetAllMime()
{
    return std::vector<MimePart_t>();
}

std::string FCurlHttpResponse::GetContentType()
{
    return GetHeader("Content-Type");
}

std::string FCurlHttpResponse::GetContentAsString()
{
    return std::string((char*)Content.data());
}
