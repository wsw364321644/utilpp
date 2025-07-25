#pragma once
#include <atomic>
#include <vector>

#include <Websocket/IWebsocketClient.h>
#include <SteamID.h>
#include <steam/steammessages_auth.steamclient.pb.h>
#include "SteamMsg/SteamClient.h"
#include "SteamMsg/SteamPacketMessage.h"
#include "SteamMsg/SteamClientInternal.h"

class FSteamClient;
class FSteamAuthSession {
public:
    FSteamAuthSession(FSteamClient* inOwner) :Owner(inOwner) {}
    bool IsAuthenticated() {
        return AuthSessionStatus == ESteamClientAuthSessionStatus::Authenticated;
    }
    bool OnSteamPackageReceived(FSteamPacketMsg& msg, std::string_view bodyView);
    
    void InvalidateCurrentAuth();
    void ClearCurrentAuth();
    void Tick(float delta);

    FCommonHandlePtr BeginAuthSessionViaCredentials(std::string_view, std::string_view, ISteamClient::FSteamRequestFinishedDelegate, std::error_code&);
    FCommonHandlePtr SendSteamGuardCode(std::string_view, ISteamClient::FSteamRequestFinishedDelegate, std::error_code&);


    FSteamClient* Owner;
    std::atomic< ESteamClientAuthSessionStatus> AuthSessionStatus{ ESteamClientAuthSessionStatus ::Unauthorized };

    uint64_t PollJobID{ 0 };
    uint64_t DeviceCodeJobID{ 0 };
    uint64_t EmailCodeJobID{ 0 };
    //from session
    uint64_t ClientID{ 0 };
    std::string AgreementSessionUrl;
    std::string ChallengeUrl;
    std::string RequestID;
    std::string WeakToken;
    typedef struct AllowedConfirmation_t {
        utilpp::steam::EAuthSessionGuardType AuthSessionGuardType;
        std::string AssociatedMessage;
    }AllowedConfirmation_t;
    std::unordered_map<utilpp::steam::EAuthSessionGuardType,AllowedConfirmation_t> AllowedConfirmations;
    std::unordered_set<ESteamClientAuthSessionGuardType> OutAllowedConfirmations;
    //from poll
    std::string NewGuardData;
    std::string AccessToken;
    std::string RefreshToken;
    std::string AccountName;

    ////cache

    std::string Password;
    std::shared_ptr<SteamRequestHandle_t> AuthRequestHandlePtr;
    ISteamClient::FSteamRequestFinishedDelegate SteamRequestFailedDelegate;
    std::shared_ptr<SteamRequestHandle_t> SteamGuardCodeRequestHandlePtr;
    ISteamClient::FSteamRequestFinishedDelegate SteamGuardCodeDelegate;
    std::string SteamGuardCode;
private:
    bool ReadAccountCache();
    void InvalidateAccountCache(std::string_view);
    bool UpdateAccountCache();
    void PollAuthSessionStatus();
    void OnGetPasswordRSAPublicKeyResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnGetPasswordRSAPublicKeyResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnBeginAuthSessionViaCredentialsResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnBeginAuthSessionViaCredentialsResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnPollAuthSessionStatusResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnPollAuthSessionStatusResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnUpdateAuthSessionWithSteamGuardCodeResponse(FSteamPacketMsg& msg, std::string_view bodyView);

};