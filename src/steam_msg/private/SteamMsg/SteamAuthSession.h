#pragma once
#include <atomic>
#include <vector>

#include <Websocket/IWebsocketClient.h>
#include <SteamID.h>
#include <steam/steammessages_auth.steamclient.pb.h>
#include "SteamMsg/SteamClient.h"
#include "SteamMsg/SteamPacketMessage.h"
#include "SteamMsg/SteamClientInternal.h"

enum class EAuthSessionStatus {
    Unauthorized,
    WaittingConnection,
    GettingPasswordRSAPublicKey,
    CreattingSeesion,
    PollingAuthSessionStatus,
    Authenticated,
};

class FSteamClient;
class FSteamAuthSession {
public:
    FSteamAuthSession(FSteamClient* inOwner) :Owner(inOwner) {}
    bool IsAuthenticated() {
        return AuthSessionStatus == EAuthSessionStatus::Authenticated;
    }
    bool OnSteamPackageReceived(FSteamPacketMsg& msg, std::string_view bodyView);
    void InputSteamGuardCode(std::string_view code);
    void Tick(float delta);
    FCommonHandlePtr BeginAuthSessionViaCredentials(std::string_view, std::string_view, ISteamClient::FSteamRequestFailedDelegate, std::error_code&);
    ISteamClient::FSteamRequestFailedDelegate SteamRequestFailedDelegate;
    FSteamClient* Owner;
    std::atomic< EAuthSessionStatus> AuthSessionStatus{ EAuthSessionStatus ::Unauthorized };

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

    //from poll
    std::string NewGuardData;
    std::string AccessToken;
    std::string RefreshToken;
    std::string AccountName;

    ////cache
    std::string Account;
    std::string Password;
    std::shared_ptr<SteamRequestHandle_t> AuthRequestHandlePtr;
    std::atomic_bool bHasSteamGuardCode;
    std::string SteamGuardCode;
private:
    void PollAuthSessionStatus();
    void OnGetPasswordRSAPublicKeyResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnGetPasswordRSAPublicKeyResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnBeginAuthSessionViaCredentialsResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnBeginAuthSessionViaCredentialsResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnPollAuthSessionStatusResponse(FSteamPacketMsg& msg, std::string_view bodyView);
    bool OnPollAuthSessionStatusResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView);
    void OnUpdateAuthSessionWithSteamGuardCodeResponse(FSteamPacketMsg& msg, std::string_view bodyView);

};