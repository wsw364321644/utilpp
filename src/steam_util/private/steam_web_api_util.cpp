#ifdef STEAM_UTIL_HAS_NET
#include "steam_web_api_util.h"
#include <nlohmann/json.hpp>

bool SteamRegisterKey(HttpManagerPtr pHttpManager, std::string_view sessionID, std::string_view key, FSteamRegisterKeyDelagate Delagate) {
    if (!pHttpManager) {
        return false;
    }
    auto pReq=pHttpManager->NewRequest();
    if (!pReq) {
        return false;
    }
    pReq->SetURL(R"(https://store.steampowered.com/account/ajaxregisterkey/)");
    pReq->SetVerb(VERB_POST);
    nlohmann::json json{ nlohmann::json::value_t::object };
    json["product_key"] = key;
    json["sessionid"] = sessionID;
    pReq->SetContentAsString(json.dump());
    pReq->OnProcessRequestComplete() =
        [=](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
        Delagate(res);
        };
    return pHttpManager->ProcessRequest(pReq);
}




#endif