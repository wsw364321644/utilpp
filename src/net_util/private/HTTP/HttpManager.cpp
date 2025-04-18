#include "HTTP/HttpManager.h"
#include "HTTP/CurlHttpManager.h"
std::unordered_map<std::u8string_view, std::shared_ptr<NamedManagerData_t>, string_hash> IHttpManager::FnCreates;


HttpManagerPtr IHttpManager::GetNamedManager(std::u8string_view name) {
    auto itr = FnCreates.find(name);
    if (itr == FnCreates.end()) {
        return nullptr;
    }
    auto& [_name,pdata]=*itr;
    auto& data = *pdata;
    auto expected = data.Ptr.load();
    if (!expected) {
        auto desired=data.CreateFn();
        data.Ptr.compare_exchange_strong(expected, desired);
        return data.Ptr.load();
    }
    else {
        return expected;
    }
}

HttpManagerPtr IHttpManager::GetNamedManager(const char* name) {
    return GetNamedManager((const char8_t*)name);
}