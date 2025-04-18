#include "HTTP/CurlHttpRequest.h"
#include "HTTP/CurlHttpManager.h"
#include <algorithm>
#include <iterator>
#include <uri.h>
#include <string_view>
#include <ranges>
#include <LoggerHelper.h>


template <typename Out>
void split(const std::string& str, const char* delimstr, Out result) {
    //using std::operator""sv;
    //constexpr auto delim{ "&"sv};
    //auto delim = std::string_view{ delimstr, strlen(delimstr) };
    //for (const auto word : std::views::split(s, delimstr)) {
        //*result++ = std::string(std::string_view(word));
        //std::string(std::string_view(word));
    //}


    size_t pos = 0, cursor = 0;
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

std::string_view FCurlHttpRequest::GetURL()
{
    return URL;
}

std::string_view FCurlHttpRequest::GetURLParameter(const std::string_view ParameterName)
{
    ParsedURL_t res;
    ParseUrl(URL, &res);
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

std::string_view FCurlHttpRequest::GetHeader(const std::string_view HeaderName)
{
    auto itr = Headers.find(std::string(HeaderName));
    if (itr != Headers.end()) {
        return itr->second;
    }
    return std::string();
}

std::vector<std::string> FCurlHttpRequest::GetAllHeaders()
{
    std::vector<std::string> out;
    std::transform(Headers.begin(), Headers.end(), std::back_inserter(out), [](auto& kv) { return kv.first + ":" + kv.second; });
    return out;
}

std::string_view FCurlHttpRequest::GetContentType()
{
    return GetHeader("Content-Type");
}


int64_t FCurlHttpRequest::GetContentLength()
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

std::string_view FCurlHttpRequest::GetVerb()
{
    return Verb;
}

void FCurlHttpRequest::SetVerb(const std::string_view _Verb)
{
    Verb = _Verb;
}

void FCurlHttpRequest::SetURL(const std::string_view _URL)
{
    URL = _URL;
}

void FCurlHttpRequest::SetHost(const std::string_view _Host)
{
    Host = _Host;
}

void FCurlHttpRequest::SetPath(const std::string_view _Path)
{
    Path = _Path;
}

void FCurlHttpRequest::SetScheme(const std::string_view _Scheme)
{
    Scheme = _Scheme;
}

void FCurlHttpRequest::SetPortNum(uint32_t _Port)
{
    Port = _Port;
}

void FCurlHttpRequest::SetQuery(const std::string_view QueryName, const std::string_view QueryValue)
{
    Queries[std::string(QueryName)] = QueryValue;
}

void FCurlHttpRequest::SetContent(const std::vector<uint8_t>& ContentPayload)
{
    Content = ContentPayload;
}

void FCurlHttpRequest::SetContentAsString(const std::string_view ContentString)
{
    Content.assign(ContentString.begin(), ContentString.end());
}


void FCurlHttpRequest::SetHeader(const std::string_view HeaderName, const std::string_view HeaderValue)
{
    Headers[std::string(HeaderName)] = HeaderValue;
}

void FCurlHttpRequest::AppendToHeader(const std::string_view HeaderName, const std::string_view AdditionalHeaderValue)
{
    auto itr = Headers.find(std::string(HeaderName));
    if (itr == Headers.end()) {
        SetHeader(HeaderName, AdditionalHeaderValue);
    }
    else {
        itr->second.append(",");
        itr->second.append(AdditionalHeaderValue);
    }
}

void FCurlHttpRequest::SetMimePart(InMimePart_t part)
{
    std::string str(part.FileData);
    MimeParts.emplace_back(MimePart{ .Name = std::string(part.Name) ,.Data= std::string(part.Data),.FileName= std::string(part.FileName),.FileData = std::string(part.FileData), });
}


void FCurlHttpRequest::SetRange(uint64_t begin, uint64_t end)
{
    if (end < begin) {
        return;
    }
    auto itr = Ranges.begin();
    for (; itr != Ranges.end();) {
        if (begin <= itr->first) {
            if (end < itr->first) {
                Ranges.insert(itr, std::pair(begin, end));
                break;
            }
            else {
                end = std::max(itr->second, end);
                itr = Ranges.erase(itr);
                continue;
            }
        }
        else if (begin > itr->second) {
            std::advance(itr, 1);
        }
        else {
            if (end <= itr->second) {
                break;
            }
            else {
                begin = std::min(itr->first, begin);
                itr = Ranges.erase(itr);
                continue;
            }
        }
    }
    if (itr == Ranges.end()) {
        Ranges.push_back(std::pair(begin, end));
    }
}

bool FCurlHttpRequest::ProcessRequest()
{
    return Manager->ProcessRequest(shared_from_this());
}

void FCurlHttpRequest::Clear()
{
    Content.clear();
    Headers.clear();
    Queries.clear();
    Verb.clear();
    URL.clear();
    Host.clear();
    Path.clear();
    Scheme.clear();
    Port = std::numeric_limits<uint32_t>::max();
    RequestID = 0;
    CompletionStatus = EHttpRequestStatus::NotStarted;
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
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}:{}", (void*)this, std::string(DebugInfo, DebugInfoSize));
    }
    break;

    case CURLINFO_HEADER_IN:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Received header ({} bytes)", (void*)this, DebugInfoSize);
    }
    break;
    case CURLINFO_HEADER_OUT:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Sent header ({} bytes) - {}", (void*)this, DebugInfoSize, std::string(DebugInfo, DebugInfoSize));
    }
    break;

    case CURLINFO_DATA_IN:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Received data ({} bytes)", (void*)this, DebugInfoSize);
    }
    break;
    case CURLINFO_DATA_OUT:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Sent data ({} bytes)", (void*)this, DebugInfoSize);
    }
    break;

    case CURLINFO_SSL_DATA_IN:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Received SSL data ({} bytes)", (void*)this, DebugInfoSize);
    }
    break;
    case CURLINFO_SSL_DATA_OUT:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: Sent SSL data ({} bytes)", (void*)this, DebugInfoSize);
    }
    break;
    default:
    {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "{}: DebugCallback: Unknown DebugInfoType={} (DebugInfoSize: {} bytes)", (void*)this, (int32_t)DebugInfoType, DebugInfoSize);
    }
    break;
    }

    return 0;
}

size_t FCurlHttpRequest::StaticDebugCallback(CURL* Handle, curl_infotype DebugInfoType, char* DebugInfo, size_t DebugInfoSize, void* UserData)
{
    auto creq = (FCurlHttpRequest*)UserData;
    return creq->DebugCallback(Handle, DebugInfoType, DebugInfo, DebugInfoSize, UserData);
}


std::string_view FCurlHttpResponse::GetHeader(const std::string_view HeaderName)
{
    auto itr = Headers.find(std::string(HeaderName));
    if (itr != Headers.end()) {
        return std::string_view(itr->second);
    }
    return std::string_view();
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

std::string_view FCurlHttpResponse::GetContentType()
{
    return GetHeader("Content-Type");
}

const std::vector<uint8_t>& FCurlHttpResponse::GetContent()
{
    return Content;
}

std::u8string_view FCurlHttpResponse::GetContentAsString()
{
    return std::u8string_view((const char8_t*)Content.data(), Content.size());
}

void FCurlHttpResponse::SetContentBuf(void* Ptr, int64_t Len)
{
    UserBuf = (char*)Ptr;
    UserBufLen = Len;
}

void FCurlHttpResponse::ContentAppend(char* Data, size_t Len)
{
    auto iTotalBytesRead = TotalBytesRead.load();
    if (UserBuf) {
        auto writelen = UserBufLen >= iTotalBytesRead + Len ? Len : UserBufLen - iTotalBytesRead;
        if (writelen > 0) {
            memcpy(UserBuf + iTotalBytesRead, Data, writelen);
        }
    }
    else {
        Content.reserve(iTotalBytesRead + Len);
        auto itr = Content.begin();
        itr += iTotalBytesRead;
        Content.insert(itr, Data, Data + Len + 1);
    }
    TotalBytesRead += Len;
}

void FCurlHttpResponse::Clear()
{
    Headers.clear();
    Content.clear();
    bSucceeded = false;
    bIsReady = false;
    HttpCode = 0;
    ContentLength = -1;
    TotalBytesRead = 0;
}
