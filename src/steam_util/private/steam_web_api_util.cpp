#ifdef STEAM_UTIL_HAS_NET
#include "steam_web_api_util.h"
#include <simdjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
namespace utilpp {
    static std::string SteamProxy;
    void SetSteamProxy(std::string_view proxy)
    {
        SteamProxy = proxy;
    }
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
        rapidjson::Document doc(rapidjson::kObjectType);
        auto& a = doc.GetAllocator();
        doc.AddMember("product_key", rapidjson::Value(key.data(), key.size(), a), a);
        doc.AddMember("sessionid", rapidjson::Value(sessionID.data(), sessionID.size(), a), a);
        rapidjson::StringBuffer buf;
        rapidjson::Writer writer(buf);
        if (!doc.Accept(writer)) {
            return nullptr;
        }
        pReq->SetContentAsString(std::string_view(buf.GetString(), buf.GetSize()));
        pReq->OnProcessRequestComplete() =
            [=](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
            Delagate(res);
            };
        if (!pHttpManager->ProcessRequest(pReq)) {
            return nullptr;
        }
        return pReq;
    }

    bool GetCMListForConnectResp_t::Parse(FCharBuffer& buf)
    {
        buf.Reverse(buf.Length() + simdjson::SIMDJSON_PADDING);
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document doc = parser.iterate(buf.Data(), buf.Length(), buf.Capacity());
        auto ores = doc["response"].get_object();
        if (ores.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        auto ares=ores["serverlist"].get_array();
        if (ares.error() != simdjson::error_code::SUCCESS) {
            return false;
        }
        EndpointDetails.clear();
        for (auto item : ares) {
            EndpointDetail_t detail;
            ores=item.get_object();
            if (ores.error() != simdjson::error_code::SUCCESS) {
                return false;
            }
            auto sres=ores["endpoint"].get_string();
            if (sres.error() != simdjson::error_code::SUCCESS) {
                return false;
            }
            auto splitText = sres.value_unsafe() | std::views::split(':');
            auto itr = splitText.begin();
            detail.Host = std::string_view(*itr);
            std::advance(itr, 1);
            auto portView = std::string_view(*itr);
            std::from_chars(portView.data(), portView.data() + portView.size(), detail.Port);

            sres = ores["type"].get_string();
            if (sres.error() != simdjson::error_code::SUCCESS) {
                return false;
            }
            detail.Type = sres.value_unsafe();

            auto ures = ores["load"].get_uint64();
            if (ures.error() != simdjson::error_code::SUCCESS) {
                return false;
            }
            detail.Load = ures.value_unsafe();

            auto dres = ores["wtd_load"].get_double();
            if (dres.error() != simdjson::error_code::SUCCESS) {
                return false;
            }
            detail.WeightedTotalDemandLoad = dres.value_unsafe();
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
        pReq->SetPath((std::string(I_STEAM_DIRECTORY_PATH) + "/GetCMListForConnect/v1").c_str());
        if (!SteamProxy.empty()) {
            pReq->SetProxyURL(SteamProxy);
        }
        pReq->OnProcessRequestComplete() =
            [=](HttpRequestPtr req, HttpResponsePtr rep, bool res) {
            if (res) {
                Delagate(res, &rep->GetContent());
            }
            else {
                FCharBuffer buf;
                Delagate(res, nullptr);
            }
            };
        if (!pHttpManager->ProcessRequest(pReq)) {
            return nullptr;
        }
        return pReq;
    }


}
#endif