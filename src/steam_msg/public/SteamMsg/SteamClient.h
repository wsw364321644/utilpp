#pragma once
#include <system_error>
#include <delegate_macros.h>
#include <handle.h>
#include <functional>
#include <unordered_set>
#include <HTTP/HttpManager.h>
#include <Websocket/IWebsocketConnectionManager.h>
#include <SteamClientDefinition.h>
#include "steam_msg_export_defs.h"


typedef struct SteamAccoutnInfo_t {
    uint64_t SteamID;
}SteamAccoutnInfo_t;

/**
* rely on IWebsocketConnectionManager
* need call Tick on IWebsocketConnectionManager external(GetWebsocketConnectionManagerSingleton()->Tick())
* rely on IHttpManager
* need call Tick on IHttpManager external(IHttpManager::GetNamedClassSingleton()->Tick())
*/


class STEAM_MSG_EXPORT ISteamClient {
public:
    typedef std::function<void(std::error_code)> FSteamRequestFinishedDelegate;
    typedef std::function<void(std::error_code)> FSteamRequestFailedDelegate;
    typedef std::function<void()> FSteamSuccessDelegate;
    virtual ~ISteamClient() = default;
    virtual bool Init(IWebsocketConnectionManager*,HttpManagerPtr) = 0;
    virtual void Disconnect() = 0;
    virtual void CancelRequest(FCommonHandlePtr) = 0;
    virtual ESteamClientLogStatus GetLoginStatus() const = 0;
    virtual ESteamClientAuthSessionStatus GetAuthSessionStatus() const = 0;
    virtual const SteamAccoutnInfo_t& GetAccoutnInfo() const = 0;
    virtual const std::unordered_set<ESteamClientAuthSessionGuardType>& GetAllowedConfirmations() const = 0;

    virtual FCommonHandlePtr Login(std::string_view, std::string_view, FSteamRequestFailedDelegate, std::error_code&) = 0;
    virtual FCommonHandlePtr SendSteamGuardCode(std::string_view, FSteamRequestFinishedDelegate, std::error_code&) = 0;
    virtual FCommonHandlePtr RegisterKey(std::string_view, FSteamRequestFinishedDelegate, std::error_code&) = 0;
    virtual void Tick(float delta) = 0;
    DEFINE_EVENT(OnConnected);
    DEFINE_EVENT_ONE_PARAM(OnDisconnected, const std::error_code&);

    DEFINE_EVENT_ONE_PARAM(OnLogin,const SteamAccoutnInfo_t&);
    DEFINE_EVENT_ONE_PARAM(OnLogout, const std::error_code&);

    DEFINE_EVENT_ONE_PARAM(OnWaitConfirmation, const std::unordered_set<ESteamClientAuthSessionGuardType>&);
};

STEAM_MSG_EXPORT ISteamClient* GetSteamClientSingleton();