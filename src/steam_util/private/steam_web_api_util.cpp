#ifdef STEAM_UTIL_HAS_NET
#include "steam_web_api_util.h"
#include <nlohmann/json.hpp>
namespace utilpp {
    HttpRequestPtr SteamRegisterKey(HttpManagerPtr pHttpManager, std::string_view sessionID, std::string_view key, FSteamRegisterKeyDelagate Delagate) {
        if (!pHttpManager) {
            return nullptr;
        }
        auto pReq = pHttpManager->NewRequest();
        if (!pReq) {
            return nullptr;
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
        if (!pHttpManager->ProcessRequest(pReq)) {
            return nullptr;
        }
        return pReq;
    }

    bool GetCMListForConnectResp_t::Parse(std::u8string_view view)
    {
        auto obj = nlohmann::json::parse(nlohmann::detail::span_input_adapter((const char*)view.data(), view.size())
            ,nullptr,false,false);
        if (obj.is_discarded()) {
            return false;
        }
        auto itr=obj.find("response");
        if (itr == obj.end()) {
            return false;
        }
        auto respItr = itr.value().find("serverlist");
        if (respItr == itr.value().end()|| !respItr.value().is_array()) {
            return false;
        }
        EndpointDetails.clear();
        for (int i = 0; i < respItr.value().size(); i++) {
            EndpointDetail_t detail;
            auto& servernode = respItr.value()[i];
            auto&Endpoint= servernode.at("endpoint").get_ref<nlohmann::json::string_t&>();
            auto splitText = Endpoint | std::views::split(':');
            auto itr = splitText.begin();
            detail.Host = std::string_view(*itr);
            std::advance(itr, 1);
            auto portView=std::string_view(*itr);
            std::from_chars(portView.data(), portView.data() + portView.size(), detail.Port);

            detail.Type= servernode.at("type").get_ref<nlohmann::json::string_t&>();
            detail.Load= servernode.at("load").get_ref<nlohmann::json::number_integer_t&>();
            detail.WeightedTotalDemandLoad= servernode.at("wtd_load").get_ref<nlohmann::json::number_float_t&>();
            EndpointDetails.emplace_back(std::move(detail));
        }
        return true;
    }

    HttpRequestPtr GetCMListForConnect(HttpManagerPtr pHttpManager, FCommonBodyDelagate Delagate)
    {
        if (!pHttpManager) {
            return nullptr;
        }
        auto pReq = pHttpManager->NewRequest();
        if (!pReq) {
            return nullptr;
        }
        pReq->SetVerb(VERB_GET);
        pReq->SetScheme(SCHEME_HTTPS);
        pReq->SetHost(STEAM_API_HOST);
        pReq->SetPath((std::string(I_STEAM_DIRECTORY_PATH)+"/GetCMListForConnect/v1").c_str());

        pReq->OnProcessRequestComplete() =
            [=](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
            Delagate(res, rep->GetContentAsString());
            };
        if (!pHttpManager->ProcessRequest(pReq)) {
            return nullptr;
        }
        return pReq;
    }


}
#endif