#pragma once
#include "IHttpRequest.h"
#include <functional>

class FHttpManager {

public:

    FHttpManager() {};
    virtual ~FHttpManager() {}

    template<class T>
    void RigisterRequsetClass() {
        FnCreate = []() -> HttpRequestPtr {
            return std::make_shared<T>();
        };
    };

    virtual HttpRequestPtr NewRequest();
    virtual bool ProcessRequest(HttpRequestPtr) = 0;
    virtual void Tick() = 0;

    std::string GetDefaultUserAgent() { return "Mozilla"; }
private:
    std::function<HttpRequestPtr()> FnCreate;
};
