#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <std_ext.h>
#include <named_class_register.h>
#include "IHttpRequest.h"
#include "net_export_defs.h"

SIMPLE_NET_EXPORT extern const char CURL_HTTP_MANAGER_NAME[];
constexpr char VERB_POST[] = "POST";
constexpr char VERB_PUT[] = "PUT";
constexpr char VERB_GET[] = "GET";
constexpr char VERB_HEAD[] = "HEAD";
constexpr char VERB_DELETE[] = "DELETE";
constexpr char SCHEME_HTTPS[] = "https";
constexpr char SCHEME_HTTP[] = "http";


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
