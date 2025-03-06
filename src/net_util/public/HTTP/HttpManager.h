#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <std_ext.h>
#include "IHttpRequest.h"
#include "net_export_defs.h"

const char CURL_HTTP_MANAGER_NAME[] = "Curl";
const char VERB_POST[] = "POST";
const char VERB_PUT[] = "PUT";
const char VERB_GET[] = "GET";
const char VERB_HEAD[] = "HEAD";
const char VERB_DELETE[] = "DELETE";

typedef std::shared_ptr<class IHttpManager> HttpManagerPtr;
typedef struct NamedManagerData_t {
    std::atomic<HttpManagerPtr> Ptr;
    std::function<HttpManagerPtr()> CreateFn;
    std::u8string Name;
}NamedManagerData_t;

class SIMPLE_NET_EXPORT IHttpManager {

public:

    IHttpManager() {};
    virtual ~IHttpManager() {}

    template<class T>
    static void RigisterNamedManager(std::u8string_view name) {
        auto pNamedManagerData=std::make_shared<NamedManagerData_t>();
        pNamedManagerData->Name = name;

        auto [pair, res] = FnCreates.try_emplace(pNamedManagerData->Name, pNamedManagerData);
        auto& [key, val] = *pair;
        pNamedManagerData->CreateFn =
            []() -> HttpManagerPtr {
            return std::make_shared<T>();
            };
    };
    static HttpManagerPtr GetNamedManager(const char* name);
    static HttpManagerPtr GetNamedManager(std::u8string_view name);
    virtual HttpRequestPtr NewRequest() = 0;
    virtual bool ProcessRequest(HttpRequestPtr) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void HttpThreadTick(float delSec) = 0;
    std::string GetDefaultUserAgent() { return "Mozilla"; }

private:
    static std::unordered_map<std::u8string_view, std::shared_ptr<NamedManagerData_t>, string_hash> FnCreates;
};
