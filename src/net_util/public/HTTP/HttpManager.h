#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <std_ext.h>
#include <named_class_register.h>
#include "IHttpRequest.h"
#include "net_export_defs.h"

const char CURL_HTTP_MANAGER_NAME[] = "Curl";
const char VERB_POST[] = "POST";
const char VERB_PUT[] = "PUT";
const char VERB_GET[] = "GET";
const char VERB_HEAD[] = "HEAD";
const char VERB_DELETE[] = "DELETE";

typedef std::shared_ptr<class IHttpManager> HttpManagerPtr;

class SIMPLE_NET_EXPORT IHttpManager:public TNamedClassRegister<IHttpManager> {

public:

    IHttpManager() {};
    virtual ~IHttpManager() {}
    virtual HttpRequestPtr NewRequest() = 0;
    virtual bool ProcessRequest(HttpRequestPtr) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void HttpThreadTick(float delSec) = 0;
    std::string GetDefaultUserAgent() { return "Mozilla"; }
};
