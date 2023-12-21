#include "HTTP/HttpManager.h"
#include "HTTP/CurlHttpManager.h"
#include "HTTP/CurlHttpRequest.h"
#include <string_buffer.h>
#include <curl/curl.h>
#include <logger.h>
#include <string>
#include <regex>

FCurlHttpManager::CurlRequestOptions_t FCurlHttpManager::CurlRequestOptions;
FCurlHttpManager::FCurlHttpManager()
{
    CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLcode::CURLE_OK) {
        return;
    }

    MultiHandle = curl_multi_init();
    if (!MultiHandle) {
        return;
    }
    ShareHandle = curl_share_init();
    if (!ShareHandle)
    {
        LOG_ERROR("Could not initialize libcurl share handle!");
        return;
    }
    curl_share_setopt(ShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(ShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(ShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);

    CurlState = READY;
}

HttpRequestPtr FCurlHttpManager::NewRequest()
{
    //return std::make_shared<CurlHttpRequest>(this);
    auto req = std::shared_ptr<FCurlHttpRequest>(new FCurlHttpRequest(this));
    req->Response = std::make_shared<FCurlHttpResponse>(req.get());
    return req;
}

bool FCurlHttpManager::ProcessRequest(HttpRequestPtr req)
{

    auto creq = std::dynamic_pointer_cast<FCurlHttpRequest>(req);

    auto resitr = std::find(Reqs.cbegin(), Reqs.cend(), creq);
    if (resitr != Reqs.cend()) {
        return false;
    }
    SetupLocalRequest(creq);

    //auto ThreadedRequest = std::make_shared<FCurlHttpRequest>(*creq);
    auto ThreadedRequest = creq;
    if (!SetupRequest(ThreadedRequest))
    {
        LOG_WARNING("Could not set libcurl options for easy handle, processing HTTP request failed. Increase verbosity for additional information.");
        // No response since connection failed
        creq->Response = NULL;
        // Cleanup and call delegate
        FinishRequest(creq);

        return false;
    }


    // Response object to handle data that comes back after starting this request
    Reqs.push_back(creq);
    LOG_DEBUG("{}: threaded {}", (void*)creq.get(), (void*)ThreadedRequest.get());
    {
        std::scoped_lock(ReqMutex);
        RunningRequests.push_back(ThreadedRequest);
    }
    LOG_DEBUG("{}: request(easy handle : {}) has been added to threaded queue for processing", (void*)ThreadedRequest.get(), (void*)ThreadedRequest->EasyHandle);
    return true;
}

void FCurlHttpManager::Tick()
{
    std::set<CurlHttpRequestPtr> LocalFinishedRequests;
    {
        std::scoped_lock lock(ReqMutex);
        LocalFinishedRequests.swap(FinishedRequests);
    }
    for (auto& FinishedRequest : LocalFinishedRequests) {
        auto res_itr = std::find(std::begin(Reqs), std::end(Reqs), FinishedRequest);

        if (res_itr == std::end(Reqs)) {
            LOG_ERROR("finished threaded request cant found {}", (void*)FinishedRequest.get());
            continue;
        }
        FinishRequest(*res_itr); 
        Reqs.erase(res_itr);
    }

    std::list<CurlDownloadProgress_t> LocalRunningProgressList;
    {
        std::scoped_lock ProgressLock(ProgressMutex);
        LocalRunningProgressList.swap(RunningProgressList);
    }
    for (auto& LocalRunningProgress : LocalRunningProgressList) {
        auto res_itr = std::find_if(std::cbegin(Reqs), std::cend(Reqs), [&LocalRunningProgress](const CurlHttpRequestPtr& ptr)->bool {
            return ptr.get() == LocalRunningProgress.HttpReq;
            });
        if (res_itr == std::cend(Reqs)) {
            continue;
        }
        auto& localReq = *res_itr;
        localReq->OnRequestProgress()(localReq, LocalRunningProgress.OldSize, LocalRunningProgress.NewSize, LocalRunningProgress.HttpReq->Response->GetContentLength());
    }
}

void FCurlHttpManager::HttpThreadTick()
{

    HttpThreadAddTask();
    if (RunningThreadedRequests.size() > 0)
    {
        int RunningRequestNum = -1;

        curl_multi_perform(MultiHandle, &RunningRequestNum);


        // read more info if number of requests changed or if there's zero running
        // (note that some requests might have never be "running" from libcurl's point of view)
        if (RunningRequestNum == 0 || RunningRequestNum != RunningThreadedRequests.size())
        {
            for (;;)
            {
                int MsgsStillInQueue = 0;	// may use that to impose some upper limit we may spend in that loop
                CURLMsg* Message = curl_multi_info_read(MultiHandle, &MsgsStillInQueue);

                if (Message == NULL)
                {
                    break;
                }

                // find out which requests have completed
                if (Message->msg == CURLMSG_DONE)
                {
                    CURL* CompletedHandle = Message->easy_handle;
                    curl_multi_remove_handle(MultiHandle, CompletedHandle);

                    auto multiHandleItr = HandlesToRequests.find(CompletedHandle);
                    if (multiHandleItr != HandlesToRequests.end())
                    {
                        CurlHttpRequestPtr CurlRequest = multiHandleItr->second;
                        auto itr = std::find(RunningThreadedRequests.begin(), RunningThreadedRequests.end(), CurlRequest);
                        if (itr != RunningThreadedRequests.end()) {
                            (*itr)->bCompleted = true;
                            (*itr)->CurlCompletionResult = Message->data.result;
                            {
                                std::scoped_lock lock(ReqMutex);
                                FinishedRequests.insert(*itr);
                            }
                            LOG_INFO("Request {} (easy handle:{}) has completed (code:{}) and has been marked as such", (void*)CurlRequest.get(), (void*)CompletedHandle, CurlRequest->GetResponse()->GetResponseCode());
                        }
                        else {
                            LOG_WARNING("Could not find RunningThreadedRequests for completed request (easy handle: {})", (void*)CompletedHandle);
                        }
                        RunningThreadedRequests.erase(itr);
                        HandlesToRequests.erase(multiHandleItr);
                    }
                    else
                    {
                        LOG_WARNING("Could not find mapping for completed request (easy handle: {})", (void*)CompletedHandle);
                    }
                }
            }
        }
    }
}


size_t FCurlHttpManager::UploadCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq)
{
    creq->TimeSinceLastResponse = 0.0f;

    size_t SizeToSend = creq->Content.size() - creq->BytesSent;
    size_t SizeToSendThisTime = 0;

    if (SizeToSend != 0)
    {
        SizeToSendThisTime = std::min(SizeToSend, SizeInBlocks * BlockSizeInBytes);
        if (SizeToSendThisTime != 0)
        {
            // static cast just ensures that this is uint8* in fact
            memcpy(Ptr, creq->Content.data() + creq->BytesSent, SizeToSendThisTime);
            creq->BytesSent += SizeToSendThisTime;
        }
    }
    LOG_DEBUG("{}: UploadCallback: {} bytes out of {} sent. (SizeInBlocks={}, BlockSizeInBytes={}, SizeToSend={}, SizeToSendThisTime={} (<-this will get returned from the callback))",
        (void*)creq, creq->BytesSent, creq->Content.size(), SizeInBlocks, BlockSizeInBytes, SizeToSend, SizeToSendThisTime
    );
    return SizeToSendThisTime;
}

size_t FCurlHttpManager::ReceiveResponseHeaderCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq)
{
    auto cresp = creq->Response;
    if (cresp.get())
    {
        creq->TimeSinceLastResponse = 0.0f;

        uint32_t HeaderSize = SizeInBlocks * BlockSizeInBytes;
        if (HeaderSize > 0 && HeaderSize <= CURL_MAX_HTTP_HEADER)
        {
            std::vector<char> AnsiHeader;
            AnsiHeader.resize(HeaderSize + 1);

            memcpy(AnsiHeader.data(), Ptr, HeaderSize);
            AnsiHeader[HeaderSize] = 0;

            std::string Header(AnsiHeader.data());
            Header = std::regex_replace(Header, std::regex(R"(\n)"), "");
            Header = std::regex_replace(Header, std::regex(R"(\r)"), "");

            LOG_DEBUG("ReceiveResponseHeaderCallback {}: Received response header '{}'.", (void*)cresp.get(), Header);
            std::string HeaderKey, HeaderValue;
            auto i = Header.find(":");
            if (i != std::string::npos)
            {
                HeaderKey = Header.substr(0, i);
                HeaderValue = Header.substr(i + 1, Header.size());
                auto itr = cresp->Headers.find(HeaderKey);
                if (itr != cresp->Headers.end()) {
                    cresp->Headers[HeaderKey].append(", ");
                    cresp->Headers[HeaderKey].append(HeaderValue);
                }
                else {
                    cresp->Headers[HeaderKey] = HeaderValue;
                }

                //Store the content length so OnRequestProgress() delegates have something to work with
                if (HeaderKey == TEXT("Content-Length"))
                {
                    cresp->ContentLength = std::atoi(HeaderValue.c_str());
                }
            }
            else {
                cresp->Headers[HeaderKey] = "";
            }
            return HeaderSize;
        }
        else
        {
            LOG_WARNING("{}: Could not process response header for request - header size ({}) is invalid.", (void*)cresp.get(), HeaderSize);
        }
    }
    else
    {
        LOG_WARNING("{}:  Could not download response header for request - response not valid.", (void*)cresp.get());
    }

    return 0;
}

size_t FCurlHttpManager::ReceiveResponseBodyCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, FCurlHttpRequest* creq)
{
    auto Response = creq->Response;

    if (Response.get())
    {
        creq->TimeSinceLastResponse = 0.0f;

        int64_t SizeToDownload = SizeInBlocks * BlockSizeInBytes;

        LOG_DEBUG("ReceiveResponseBodyCallback {}: {} bytes out of {} received. (SizeInBlocks={}, BlockSizeInBytes={}, Response->TotalBytesRead={}, Response->GetContentLength()={}, SizeToDownload={} (<-this will get returned from the callback))",
            (void*)Response.get(), Response->TotalBytesRead + SizeToDownload, Response->GetContentLength(),
            SizeInBlocks, BlockSizeInBytes, Response->GetContentBytesRead(), Response->GetContentLength(), SizeToDownload
        );

        // note that we can be passed 0 bytes if file transmitted has 0 length
        if (SizeToDownload > 0)
        {
            auto oldsize = Response->GetContentBytesRead();
            Response->ContentAppend((char*)Ptr, SizeToDownload);
            {
                std::scoped_lock(ProgressMutex);
                RunningProgressList.emplace_back(creq, oldsize, Response->TotalBytesRead);
            }
            return SizeToDownload;
        }
    }
    else
    {
        LOG_WARNING("{}: Could not download response data for request - response not valid.", (void*)Response.get());
    }
    return 0;	// request will fail with write error if we had non-zero bytes to download
}

size_t FCurlHttpManager::StaticUploadCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData)
{
    FCurlHttpRequest* creq = (FCurlHttpRequest*)UserData;
    return creq->Manager->UploadCallback(Ptr, SizeInBlocks, BlockSizeInBytes, creq);
}

size_t FCurlHttpManager::StaticReceiveResponseHeaderCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData)
{
    FCurlHttpRequest* creq = (FCurlHttpRequest*)UserData;
    return creq->Manager->ReceiveResponseHeaderCallback(Ptr, SizeInBlocks, BlockSizeInBytes, creq);
}

size_t FCurlHttpManager::StaticReceiveResponseBodyCallback(void* Ptr, size_t SizeInBlocks, size_t BlockSizeInBytes, void* UserData)
{
    FCurlHttpRequest* creq = (FCurlHttpRequest*)UserData;
    return creq->Manager->ReceiveResponseBodyCallback(Ptr, SizeInBlocks, BlockSizeInBytes, creq);
}

void FCurlHttpManager::HttpThreadAddTask()
{
    int32_t oldsize = RunningThreadedRequests.size();
    {
        std::scoped_lock lock(ReqMutex);
        if (!RunningRequests.size()) {
            return;
        }
        RunningThreadedRequests.splice(RunningThreadedRequests.end(), RunningRequests);
    }
    auto itr = RunningThreadedRequests.begin();
    for (std::advance(itr, oldsize); itr != RunningThreadedRequests.end(); ) {
        CURLMcode AddResult = curl_multi_add_handle(MultiHandle, (*itr)->EasyHandle);
        (*itr)->CurlAddToMultiResult = AddResult;

        if (AddResult != CURLM_OK)
        {
            LOG_WARNING("Failed to add easy handle {} to multi handle with code {}", (*itr)->EasyHandle, (int)AddResult);
            RunningThreadedRequests.erase(itr++);
            continue;
        }
        else {
            HandlesToRequests[(*itr)->EasyHandle] = (*itr);
            itr++;
        }
    }
}

bool FCurlHttpManager::SetupLocalRequest(CurlHttpRequestPtr creq)
{
    CURLU* urlp{ nullptr };
    CURLUcode rc{ CURLUcode::CURLUE_OK };
    char* url{ nullptr };
    // Mark as in-flight to prevent overlapped requests using the same object
    creq->CompletionStatus = EHttpRequestStatus::Processing;

    if (creq->URL.empty()) {
        if (creq->Host.empty()) {
            return false;
        }
        urlp = curl_url();
        //rc = curl_url_set(urlp, CURLUPART_FRAGMENT, "anchor", 0);
        //rc = curl_url_set(urlp, CURLUPART_USER, "john", 0);
        //rc = curl_url_set(urlp, CURLUPART_PASSWORD, "doe", 0);
        //rc = curl_url_set(urlp, CURLUPART_ZONEID, "eth0", 0);
        rc = curl_url_set(urlp, CURLUPART_HOST, creq->Host.c_str(), 0);
        if (rc != CURLUcode::CURLUE_OK) {
            goto end;
        }
        if (!creq->Scheme.empty()) {
            rc = curl_url_set(urlp, CURLUPART_SCHEME, creq->Scheme.c_str(), 0);
            if (rc != CURLUcode::CURLUE_OK) {
                goto end;
            }
        }
        if (!creq->Path.empty()) {
            rc = curl_url_set(urlp, CURLUPART_PATH, creq->Path.c_str(), 0);
            if (rc != CURLUcode::CURLUE_OK) {
                goto end;
            }
        }
        if (creq->Port < std::numeric_limits<uint16_t>::max()) {
            rc = curl_url_set(urlp, CURLUPART_PORT, std::to_string(creq->Port).c_str(), 0);
            if (rc != CURLUcode::CURLUE_OK) {
                goto end;
            }
        }
        for (auto& pair : creq->Queries) {
            rc = curl_url_set(urlp, CURLUPART_QUERY, (pair.first + "=" + pair.second).c_str(), CURLU_APPENDQUERY | CURLU_URLENCODE);
            if (rc != CURLUcode::CURLUE_OK) {
                goto end;
            }
        }

        rc = curl_url_get(urlp, CURLUPART_URL, &url, 0);
        if (rc != CURLUcode::CURLUE_OK) {
            goto end;
        }
        creq->URL = url;
    }

end:
    if (urlp) {
        curl_url_cleanup(urlp);
    }
    if (url) {
        curl_free(url);
    }
    if (rc == CURLUcode::CURLUE_OK) {
        return true;
    }
    return false;
}

bool FCurlHttpManager::SetupRequest(CurlHttpRequestPtr creq)
{
    InitRequest(creq);
    auto CurlAddToMultiResult = CURLM_OK;

    if (!creq->Mime) {
        creq->Mime = curl_mime_init(creq->EasyHandle);
    }
    // default no verb to a GET
    if (creq->Verb.empty())
    {
        creq->Verb = "GET";
    }

    LOG_DEBUG("{}: URL='{}'", (void*)creq.get(), creq->GetURL());
    LOG_DEBUG("{}: Verb='{}'", (void*)creq.get(), creq->GetVerb());
    LOG_DEBUG("{}: Payload size= {}", (void*)creq.get(), creq->GetContentLength());


    if (creq->URL.empty())
    {
        LOG_INFO("{}: Cannot process HTTP request: URL is empty", (void*)creq.get());
        return false;
    }

    curl_easy_setopt(creq->EasyHandle, CURLOPT_URL, creq->URL.c_str());

    // set up verb (note that Verb is expected to be uppercase only)
    if (creq->Verb == TEXT("POST"))
    {
        std::vector<MimePart_t> AllMime = creq->GetAllMime();
        const int32_t NumAllMime = AllMime.size();
        if (NumAllMime) {
            for (int32_t Idx = 0; Idx < NumAllMime; ++Idx)
            {
                curl_mimepart* part;
                LOG_DEBUG("{}: MineName='{}'", (void*)creq.get(), AllMime[Idx].Name);

                part = curl_mime_addpart(creq->Mime);
                curl_mime_name(part, AllMime[Idx].Name.c_str());
                curl_mime_data(part, AllMime[Idx].Data.c_str(), CURL_ZERO_TERMINATED);
            }
            curl_easy_setopt(creq->EasyHandle, CURLOPT_CUSTOMREQUEST, creq->Verb.c_str());
            curl_easy_setopt(creq->EasyHandle, CURLOPT_MIMEPOST, creq->Mime);
        }
        else {
            curl_easy_setopt(creq->EasyHandle, CURLOPT_POST, 1L);
            curl_easy_setopt(creq->EasyHandle, CURLOPT_POSTFIELDS, creq->Content.data());
            curl_easy_setopt(creq->EasyHandle, CURLOPT_POSTFIELDSIZE, creq->Content.size());
            // content-length should be present https://www.rfc-editor.org/rfc/rfc9110.html#name-content-length
            if (creq->GetHeader("Content-Length").empty())
            {
                creq->SetHeader("Content-Length", std::to_string(creq->Content.size()));
            }
        }
    }
    else if (creq->Verb == TEXT("PUT"))
    {
        curl_easy_setopt(creq->EasyHandle, CURLOPT_UPLOAD, 1L);
        // this pointer will be passed to read function
        curl_easy_setopt(creq->EasyHandle, CURLOPT_READDATA, creq.get());
        curl_easy_setopt(creq->EasyHandle, CURLOPT_READFUNCTION, FCurlHttpManager::StaticUploadCallback);
        curl_easy_setopt(creq->EasyHandle, CURLOPT_INFILESIZE, creq->Content.size());

        // reset the counter
        creq->BytesSent = 0;
    }
    else if (creq->Verb == TEXT("GET"))
    {
        // technically might not be needed unless we reuse the handles
        curl_easy_setopt(creq->EasyHandle, CURLOPT_HTTPGET, 1L);
    }
    else if (creq->Verb == TEXT("HEAD"))
    {
        curl_easy_setopt(creq->EasyHandle, CURLOPT_NOBODY, 1L);
        //CURLOPT_HEADER  header would return by body callback(StaticReceiveResponseBodyCallback)
        //curl_setopt(creq->EasyHandle, CURLOPT_HEADER, 1);
    }
    else if (creq->Verb == TEXT("DELETE"))
    {
        curl_easy_setopt(creq->EasyHandle, CURLOPT_CUSTOMREQUEST, creq->Verb.c_str());
        curl_easy_setopt(creq->EasyHandle, CURLOPT_POSTFIELDS, creq->Content.data());
        curl_easy_setopt(creq->EasyHandle, CURLOPT_POSTFIELDSIZE, creq->Content.size());
    }
    else
    {
        LOG_ERROR("Unsupported verb '{}', can be perhaps added with CURLOPT_CUSTOMREQUEST", creq->Verb);
    }

    // set up header function to receive response headers
    curl_easy_setopt(creq->EasyHandle, CURLOPT_HEADERDATA, creq.get());
    curl_easy_setopt(creq->EasyHandle, CURLOPT_HEADERFUNCTION, FCurlHttpManager::StaticReceiveResponseHeaderCallback);

    // set up write function to receive response payload
    curl_easy_setopt(creq->EasyHandle, CURLOPT_WRITEDATA, creq.get());
    curl_easy_setopt(creq->EasyHandle, CURLOPT_WRITEFUNCTION, FCurlHttpManager::StaticReceiveResponseBodyCallback);

    // set up headers
    if (creq->GetHeader("User-Agent").empty())
    {
        creq->SetHeader("User-Agent", GetDefaultUserAgent());
    }

    // Add "Pragma: no-cache" to mimic WinInet behavior
    if (creq->GetHeader("Pragma").empty())
    {
        creq->SetHeader("Pragma", "no-cache");
    }

    // Remove "Expect: 100-continue" since this is supposed to cause problematic behavior on Amazon ELB (and WinInet doesn't send that either)
    // (also see http://www.iandennismiller.com/posts/curl-http1-1-100-continue-and-multipartform-data-post.html , http://the-stickman.com/web-development/php-and-curl-disabling-100-continue-header/ )
    if (creq->GetHeader("Expect").empty())
    {
        creq->SetHeader("Expect", "");
    }

    std::vector<std::string> AllHeaders = creq->GetAllHeaders();
    const int32_t NumAllHeaders = AllHeaders.size();
    for (int32_t Idx = 0; Idx < NumAllHeaders; ++Idx)
    {
        LOG_DEBUG("{}: Header='{}'", (void*)creq.get(), AllHeaders[Idx]);

        creq->HeaderList = curl_slist_append(creq->HeaderList, AllHeaders[Idx].c_str());
    }

    if (creq->HeaderList)
    {
        curl_easy_setopt(creq->EasyHandle, CURLOPT_HTTPHEADER, creq->HeaderList);
    }

    if (creq->Ranges.size()) {
        CharBuffer buf;
        for (auto& range : creq->Ranges) {
            if (range.second == InfiniteRange) {
                buf.FormatAppend("%lu-", range.first);
            }
            else {
                buf.FormatAppend("%lu-%lu", range.first, range.second);
            }
            if (&range != &*creq->Ranges.rbegin()) {
                buf.FormatAppend(",");
            }
        }
        curl_easy_setopt(creq->EasyHandle, CURLOPT_RANGE, buf.CStr());
    }

    return true;
}



bool FCurlHttpManager::InitRequest(CurlHttpRequestPtr creq)
{
    auto& EasyHandle = creq->EasyHandle;
    EasyHandle = curl_easy_init();
#ifdef _DEBUG
    // set debug functions (FIXME: find a way to do it only if LogHttp is >= Verbose)
    curl_easy_setopt(EasyHandle, CURLOPT_DEBUGDATA, creq.get());
    curl_easy_setopt(EasyHandle, CURLOPT_DEBUGFUNCTION, FCurlHttpRequest::StaticDebugCallback);
    curl_easy_setopt(EasyHandle, CURLOPT_VERBOSE, 1L);
#endif //  _DEBUG

    curl_easy_setopt(EasyHandle, CURLOPT_SHARE, GetShareHandle());

    curl_easy_setopt(EasyHandle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    // set certificate verification (disable to allow self-signed certificates)
    if (FCurlHttpManager::CurlRequestOptions.bVerifyPeer)
    {
        curl_easy_setopt(EasyHandle, CURLOPT_SSL_VERIFYPEER, 1L);
    }
    else
    {
        curl_easy_setopt(EasyHandle, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    // allow http redirects to be followed
    curl_easy_setopt(EasyHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(EasyHandle, CURLOPT_MAXREDIRS, 5);

    // required for all multi-threaded handles
    curl_easy_setopt(EasyHandle, CURLOPT_NOSIGNAL, 1L);

    // associate with this just in case
    curl_easy_setopt(EasyHandle, CURLOPT_PRIVATE, this);

    if (FCurlHttpManager::CurlRequestOptions.bUseHttpProxy)
    {
        // guaranteed to be valid at this point
        curl_easy_setopt(EasyHandle, CURLOPT_PROXY, FCurlHttpManager::CurlRequestOptions.HttpProxyAddress.c_str());
    }

    if (FCurlHttpManager::CurlRequestOptions.bDontReuseConnections)
    {
        curl_easy_setopt(EasyHandle, CURLOPT_FORBID_REUSE, 1L);
    }

    if (FCurlHttpManager::CurlRequestOptions.CertBundlePath)
    {
        curl_easy_setopt(EasyHandle, CURLOPT_CAINFO, FCurlHttpManager::CurlRequestOptions.CertBundlePath);
    }
    else
    {
        //curl_easy_setopt(EasyHandle, CURLOPT_SSLCERTTYPE, "PEM");
#if WITH_SSL
        //curl_easy_setopt(EasyHandle, CURLOPT_SSL_CTX_FUNCTION, *sslctx_function);
#endif // #if WITH_SSL
    }
    return true;
}

void FCurlHttpManager::FinishRequest(CurlHttpRequestPtr creq)
{
    auto Response = creq->Response;
    if (creq->bCompleted)
    {
        if (Response.get())
        {
            Response->bSucceeded = (CURLE_OK == creq->CurlCompletionResult);

            // get the information
            long HttpCode = 0;
            if (!curl_easy_getinfo(creq->EasyHandle, CURLINFO_RESPONSE_CODE, &HttpCode))
            {
                Response->HttpCode = HttpCode;
            }

            curl_off_t ContentLengthDownload;
            if (!curl_easy_getinfo(creq->EasyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &ContentLengthDownload))
            {
                Response->ContentLength = ContentLengthDownload;
            }
        }
    }

    // if just finished, mark as stopped async processing
    if (Response.get())
    {
        Response->bIsReady = true;
    }

    if (Response.get() &&
        Response->bSucceeded)
    {
        const bool bDebugServerResponse = Response->GetResponseCode() >= 500 && Response->GetResponseCode() <= 503;

        //// log info about error responses to identify failed downloads
        //if (UE_LOG_ACTIVE(LogHttp, Verbose) ||
        //	bDebugServerResponse)
        //{
        //	if (bDebugServerResponse)
        //	{
        //		UE_LOG(LogHttp, Warning, TEXT("%p: request has been successfully processed. URL: %s, HTTP code: %d, content length: %d, actual payload size: %d"),
        //			this, *GetURL(), Response->HttpCode, Response->ContentLength, Response->Payload.Num());
        //	}
        //	else
        //	{
        //		UE_LOG(LogHttp, Verbose, TEXT("%p: request has been successfully processed. URL: %s, HTTP code: %d, content length: %d, actual payload size: %d"),
        //			this, *GetURL(), Response->HttpCode, Response->ContentLength, Response->Payload.Num());
        //	}

        //	TArray<FString> AllHeaders = Response->GetAllHeaders();
        //	for (TArray<FString>::TConstIterator It(AllHeaders); It; ++It)
        //	{
        //		const FString& HeaderStr = *It;
        //		if (!HeaderStr.StartsWith(TEXT("Authorization")) && !HeaderStr.StartsWith(TEXT("Set-Cookie")))
        //		{
        //			if (bDebugServerResponse)
        //			{
        //				UE_LOG(LogHttp, Warning, TEXT("%p Response Header %s"), this, *HeaderStr);
        //			}
        //			else
        //			{
        //				UE_LOG(LogHttp, Verbose, TEXT("%p Response Header %s"), this, *HeaderStr);
        //			}
        //		}
        //	}
        //}


        // Mark last request attempt as completed successfully
        creq->CompletionStatus = EHttpRequestStatus::Succeeded;
        // Call delegate with valid request/response objects
        if (creq->HttpRequestCompleteDelegate) {
            creq->HttpRequestCompleteDelegate(creq, Response, true);
        }
    }
    else
    {
        if (creq->CurlAddToMultiResult != CURLM_OK)
        {
            LOG_WARNING("{}: request failed, libcurl multi error: {} ({})", (void*)creq.get(), (void*)creq->CurlAddToMultiResult, curl_multi_strerror(creq->CurlAddToMultiResult));
        }
        else
        {
            LOG_WARNING("{}: request failed, libcurl error: {} ({})", (void*)creq.get(), (void*)creq->CurlCompletionResult, curl_easy_strerror(creq->CurlCompletionResult));
        }



        // Mark last request attempt as completed but failed
        switch (creq->CurlCompletionResult)
        {
        case CURLE_COULDNT_CONNECT:
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
            // report these as connection errors (safe to retry)
            creq->CompletionStatus = EHttpRequestStatus::Failed_ConnectionError;
            break;
        default:
            creq->CompletionStatus = EHttpRequestStatus::Failed;
        }
        // No response since connection failed
        Response = NULL;

        // Call delegate with failure
        if (creq->HttpRequestCompleteDelegate) {
            creq->HttpRequestCompleteDelegate(creq, nullptr, false);
        }
    }
    if (creq->EasyHandle)
    {
        // cleanup the handle first (that order is used in howtos)
        curl_easy_cleanup(creq->EasyHandle);

        // destroy headers list
        if (creq->HeaderList)
        {
            curl_slist_free_all(creq->HeaderList);
            creq->HeaderList = nullptr;
        }

        if (creq->Mime)
        {
            curl_mime_free(creq->Mime);
            creq->Mime = nullptr;
        }
    }
}
