#pragma once
#include <system_error>
#include <delegate_macros.h>
#include <handle.h>
#include <functional>
#include <HTTP/HttpManager.h>
#include <Websocket/IWebsocketConnectionManager.h>
#include "steam_msg_export_defs.h"

enum class ESteamClientError : int {
    SCE_OK = 0,
    SCE_InvalidInput,
    SCE_NotConnected,

    SCE_UnknowError,
    SCE_RequestErrFromServer,
    SCE_RequestTimeout,
    //Account
    SCE_NotLogin,
    SCE_AlreadyLoggedin,
    SCE_AccountLocked,
    SCE_NoWallet,
    //Purchase
    SCE_AlreadyPurchased,
    SCE_CannotRedeemCodeFromClient,
    SCE_DoesNotOwnRequiredApp,
    SCE_RestrictedCountry,
    SCE_RateLimited,
    SCE_BadActivationCode,
    SCE_DuplicateActivationCode,

    SCE_MAX,
};

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
    typedef std::function<void()> FSteamLoginDelegate;
    virtual FCommonHandlePtr Login(std::string_view, std::string_view, FSteamRequestFailedDelegate, std::error_code&) = 0;
    virtual void InputSteamGuardCode(std::string_view) = 0;
    virtual FCommonHandlePtr RegisterKey(std::string_view, FSteamRequestFinishedDelegate, std::error_code&) = 0;
    virtual void Tick(float delta) = 0;
    DEFINE_EVENT(OnConnected);
    DEFINE_EVENT_ONE_PARAM(OnDisconnected, const std::error_code&);

    DEFINE_EVENT_ONE_PARAM(OnLogin,const SteamAccoutnInfo_t&);
    DEFINE_EVENT_ONE_PARAM(OnLogout, const std::error_code&);

    DEFINE_EVENT(OnRequestSteamGuardCode);
};

STEAM_MSG_EXPORT ISteamClient* GetSteamClientSingleton();