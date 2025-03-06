#include "HTTP/HttpManager.h"
#include "HTTP/CurlHttpManager.h"
std::unordered_map<std::u8string_view, std::shared_ptr<NamedManagerData_t>, string_hash> IHttpManager::FnCreates;
enum class EInitStatus
{
    IS_None,
    IS_Inprogress,
    IS_Finished,
};
static std::atomic< EInitStatus> Status{ EInitStatus::IS_None };

HttpManagerPtr IHttpManager::GetNamedManager(std::u8string_view name) {
    if (Status== EInitStatus::IS_None) {
        EInitStatus expected{ EInitStatus::IS_None };
        if (Status.compare_exchange_strong(expected, EInitStatus::IS_Inprogress)) {
            IHttpManager::RigisterNamedManager<FCurlHttpManager>((const char8_t*)CURL_HTTP_MANAGER_NAME);
        }
    }else if (Status != EInitStatus::IS_Finished) {
        return nullptr;
    }
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