#pragma once
#include "IHttpRequest.h"
#include <functional>
#include <unordered_map>
typedef std::shared_ptr<class FHttpManager> HttpManagerPtr;
class FHttpManager {

public:

    FHttpManager() {};
    virtual ~FHttpManager() {}

    template<class T>
    static void RigisterRequsetClass(std::string name) {
        FnCreates.emplace(name,
            []() -> HttpManagerPtr {
                return std::make_shared<T>();
            });
    };
    static HttpManagerPtr GetNamedManager(std::string name) {
        auto itr=FnCreates.find(name);
        if (itr == FnCreates.end()) {
            return nullptr;
        }
        return itr->second();
    }
    virtual HttpRequestPtr NewRequest()=0;
    virtual bool ProcessRequest(HttpRequestPtr) = 0;
    virtual void Tick() = 0;

    std::string GetDefaultUserAgent() { return "Mozilla"; }
private:
    static std::unordered_map<std::string, std::function<HttpManagerPtr()>>FnCreates;
};
