#pragma once
#include <stdint.h>

enum class ESteamClientError : int {
    SCE_OK = 0,
    SCE_UnknowError,
    SCE_InvalidInput,

    //Status
    SCE_NotConnected,
    SCE_ClientStatusError,
    SCE_AlreadyRequested,
    //rpc
    SCE_RequestErrFromServer,
    SCE_RequestTimeout,
    SCE_ResponseParseErr,
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

enum class ESteamClientAuthSessionStatus {
    Unauthorized,
    WaittingConnection,
    GettingPasswordRSAPublicKey,
    CreattingSeesion,
    PollingAuthSessionStatus,
    Authenticated,
    Error,
};

enum class ESteamClientLogStatus {
    NotConnect,
    Connecting,
    Logout,
    Loggingon,
    Logon,
    Error
};

enum class ESteamClientAuthSessionGuardType : int {
    Unknown = 0,
    None = 1,
    EmailCode = 2,
    DeviceCode = 3,
    DeviceConfirmation = 4,
    EmailConfirmation = 5,
    MachineToken = 6,
    LegacyMachineAuth = 7,
};