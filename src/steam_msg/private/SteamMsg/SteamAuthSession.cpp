#include "SteamMsg/SteamAuthSession.h"
#include "SteamMsg/SteamClientImpl.h"

#include <hex.h>
#include <FunctionExitHelper.h>
#include <LoggerHelper.h>
#include <os_sock_helper.h>
#include <os_info_helper.h>
#include <SteamLanguageInternal.h>

#include <steam/steammessages_clientserver_login.pb.h>
#include <crypto_lib_rsa.h>
#include <crypto_lib_base64.h>

#include <jwt-cpp/jwt.h>

bool FSteamAuthSession::OnSteamPackageReceived(FSteamPacketMsg& msg, std::string_view bodyView)
{
    return false;
}

void FSteamAuthSession::InvalidateCurrentAuth()
{
    if (!AccountName.empty()) {
        InvalidateAccountCache(AccountName);
        ClearCurrentAuth();
    }
}
void FSteamAuthSession::ClearCurrentAuth()
{
    AccountName.clear();
    RefreshToken.clear();
    Password.clear();
    AuthRequestHandlePtr->bFinished = true;
    AuthSessionStatus = ESteamClientAuthSessionStatus::Unauthorized;

}
void FSteamAuthSession::Tick(float delta)
{
    auto CurAuthSessionStatus = AuthSessionStatus.load();
    switch (CurAuthSessionStatus) {
    case ESteamClientAuthSessionStatus::WaittingConnection: {
        if (Owner->LogStatus == ESteamClientLogStatus::Logout) {
            Owner->PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
            auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsg.Header);
            header.set_jobid_source(Owner->GetNextJobID().GetValue());
            header.set_target_job_name("Authentication.GetPasswordRSAPublicKey#1");
            Owner->PacketMsg.bProtoBuf = true;
            Owner->PacketMsg.MsgType = utilpp::steam::EMsg::ServiceMethodCallFromClientNonAuthed;
            utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request body;
            body.set_account_name(AccountName);
            auto bodyLen = body.ByteSizeLong();
            auto [bufview, bres] = Owner->PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request::SerializeToOstream, &body, std::placeholders::_1));
            if (!bres) {
                SIMPLELOG_LOGGER_ERROR(nullptr, "CAuthentication_GetPasswordRSAPublicKey_Request SerializeToOstream failed");
                return;
            }
            Owner->pWSClient->SendData(bufview.data(), bufview.size());
            auto SourceJobID = Owner->PacketMsg.GetSourceJobID();
            AuthRequestHandlePtr->SourceJobID = SourceJobID;
            Owner->JobManager.AddJob(SourceJobID,
                std::bind(&FSteamAuthSession::OnGetPasswordRSAPublicKeyResponse, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&FSteamClient::OnRequestFinished, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
            );
            AuthSessionStatus = ESteamClientAuthSessionStatus::GettingPasswordRSAPublicKey;
        }
        break;
    }
    case ESteamClientAuthSessionStatus::PollingAuthSessionStatus: {
        if (AllowedConfirmations.contains(utilpp::steam::k_EAuthSessionGuardType_DeviceConfirmation)) {
            PollAuthSessionStatus();
        }
    }
    }
}
FCommonHandlePtr FSteamAuthSession::BeginAuthSessionViaCredentials(std::string_view account, std::string_view password, ISteamClient::FSteamRequestFailedDelegate FailedDelegate, std::error_code& ec)
{
    if (AccountName == account && Password == password) {
        if (AuthSessionStatus== ESteamClientAuthSessionStatus::Authenticated) {
            ec = std::error_code(std::to_underlying(ESteamClientError::SCE_AlreadyLoggedin), SteamClientErrorCategory());
            return nullptr;
        }
        else if(AuthSessionStatus!= ESteamClientAuthSessionStatus::Unauthorized){
            ec = std::error_code(std::to_underlying(ESteamClientError::SCE_AlreadyRequested), SteamClientErrorCategory());
            return AuthRequestHandlePtr;
        }
    }
    if (AuthRequestHandlePtr) {
        AuthRequestHandlePtr->bFinished = true;
    }
    AccountName = account;
    Password = password;
    SteamRequestFailedDelegate = FailedDelegate;
    AuthRequestHandlePtr = std::make_shared<SteamRequestHandle_t>();

    if (ReadAccountCache()) {
        AuthSessionStatus = ESteamClientAuthSessionStatus::Authenticated;
        return AuthRequestHandlePtr;
    }

    AuthRequestHandlePtr->bFinished = false;
    AuthSessionStatus = ESteamClientAuthSessionStatus::WaittingConnection;
    return AuthRequestHandlePtr;
}

FCommonHandlePtr FSteamAuthSession::SendSteamGuardCode(std::string_view code, ISteamClient::FSteamRequestFinishedDelegate FinishedDelegate, std::error_code& ec)
{
    if (AuthSessionStatus != ESteamClientAuthSessionStatus::PollingAuthSessionStatus) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_ClientStatusError), SteamClientErrorCategory());
        return nullptr;
    }
    if (SteamGuardCodeRequestHandlePtr) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_AlreadyRequested), SteamClientErrorCategory());
        return SteamGuardCodeRequestHandlePtr;
    }
    SteamGuardCode = code;
    SteamGuardCodeDelegate = FinishedDelegate;

    utilpp::steam::EAuthSessionGuardType GuardType{ utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_Unknown };
    if (AllowedConfirmations.contains(utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode)) {
        GuardType = utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_DeviceCode;
    }
    if (AllowedConfirmations.contains(utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode)) {
        GuardType = utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_EmailCode;
    }
    if (GuardType == utilpp::steam::EAuthSessionGuardType::k_EAuthSessionGuardType_Unknown) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_InvalidInput), SteamClientErrorCategory());
        return nullptr;
    }

    Owner->PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsg.Header);
    header.set_jobid_source(Owner->GetNextJobID().GetValue());
    header.set_target_job_name("Authentication.UpdateAuthSessionWithSteamGuardCode#1");
    Owner->PacketMsg.bProtoBuf = true;
    Owner->PacketMsg.MsgType = utilpp::steam::EMsg::ServiceMethodCallFromClientNonAuthed;
    utilpp::steam::CAuthentication_UpdateAuthSessionWithSteamGuardCode_Request body;
    body.set_client_id(ClientID);
    body.set_steamid(Owner->SteamAccoutnInfo.SteamID);
    body.set_code(SteamGuardCode);
    body.set_code_type(GuardType);
    auto bodyLen = body.ByteSizeLong();
    auto [bufview, bres] = Owner->PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request::SerializeToOstream, &body, std::placeholders::_1));
    if (!bres) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_UnknowError), SteamClientErrorCategory());
        return nullptr;
    }
    SteamGuardCodeRequestHandlePtr = std::make_shared<SteamRequestHandle_t>();
    Owner->pWSClient->SendData(bufview.data(), bufview.size());
    SteamGuardCodeRequestHandlePtr->SourceJobID = Owner->PacketMsg.GetSourceJobID();
    Owner->JobManager.AddJob(PollJobID,
        std::bind(&FSteamAuthSession::OnUpdateAuthSessionWithSteamGuardCodeResponse, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&FSteamClient::OnRequestFinished, Owner, SteamGuardCodeRequestHandlePtr, SteamGuardCodeDelegate, std::placeholders::_1)
    );
    return SteamGuardCodeRequestHandlePtr;
}

bool FSteamAuthSession::ReadAccountCache()
{
    //int ires;
    //ires = sqlite3_clear_bindings(Owner->pSelectAccountPSO);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_reset(Owner->pSelectAccountPSO);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_bind_text(Owner->pSelectAccountPSO, 1, AccountName.data(), static_cast<int>(AccountName.size()), SQLITE_TRANSIENT);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //while (sqlite3_step(Owner->pSelectAccountPSO) != SQLITE_DONE) {
    //    int i;
    //    int num_cols = sqlite3_column_count(Owner->pSelectAccountPSO);
    //    for (i = 0; i < num_cols; i++)
    //    {
    //        switch (sqlite3_column_type(Owner->pSelectAccountPSO, i))
    //        {
    //        case (SQLITE3_TEXT):
    //            RefreshToken = (char*)sqlite3_column_text(Owner->pSelectAccountPSO, i);
    //            break;
    //        case (SQLITE_INTEGER):
    //            break;
    //        case (SQLITE_FLOAT):
    //            break;
    //        default:
    //            break;
    //        }
    //    }
    //}
    try {
        for (const auto& row : Owner->GetDBConnection()(sqlpp::select(Owner->SteamUserTable.RefreshToken)
            .from(Owner->SteamUserTable)
            .where(Owner->SteamUserTable.AccountName == AccountName))
            ) {
            RefreshToken = row.RefreshToken.value();
            break;
        }
    }
    catch (sqlpp::exception e) {
    }

    if (!RefreshToken.empty()) {
        auto decoded = jwt::decode(RefreshToken);
        if (std::chrono::system_clock::now().time_since_epoch().count() + 60 * 5 > decoded.get_expires_at().time_since_epoch().count()) {
            InvalidateAccountCache(AccountName);
            RefreshToken.clear();
        }
    }
    if (!RefreshToken.empty()) {
        return true;
    }
    return false;
}

void FSteamAuthSession::InvalidateAccountCache(std::string_view accountName)
{
    //Owner->GetDBConnection()(sqlpp::update(Owner->SteamUserTable)
    //    .set(Owner->SteamUserTable.RefreshToken = std::nullopt)
    //    .where(Owner->SteamUserTable.AccountName == accountName)
    //    );
    try {
        Owner->GetDBConnection()(sqlpp::delete_from(Owner->SteamUserTable)
            .where(Owner->SteamUserTable.AccountName == accountName)
            );
    }
    catch (sqlpp::exception e) {
    }

}

bool FSteamAuthSession::UpdateAccountCache()
{
    //int ires;
    //ires = sqlite3_clear_bindings(Owner->pInsertAccountPSO);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_reset(Owner->pInsertAccountPSO);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_bind_int64(Owner->pInsertAccountPSO, 1, Owner->SteamAccoutnInfo.SteamID);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_bind_text(Owner->pInsertAccountPSO, 2, AccountName.data(), static_cast<int>(AccountName.size()), SQLITE_STATIC);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_bind_text(Owner->pInsertAccountPSO, 3, AccessToken.data(), static_cast<int>(AccessToken.size()), SQLITE_STATIC);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_bind_text(Owner->pInsertAccountPSO, 4, RefreshToken.data(), static_cast<int>(RefreshToken.size()), SQLITE_STATIC);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}

    //while (sqlite3_step(Owner->pInsertAccountPSO) != SQLITE_DONE) {
    //}
    try {
        Owner->GetDBConnection()(sqlpp::sqlite3::insert_or_replace().into(Owner->SteamUserTable)
            .set(Owner->SteamUserTable.SteamID = Owner->SteamAccoutnInfo.SteamID,
                Owner->SteamUserTable.AccountName = AccountName,
                Owner->SteamUserTable.AccessToken = AccessToken,
                Owner->SteamUserTable.RefreshToken = RefreshToken)
            );
    }
    catch (sqlpp::exception e) {
        return false;
    }
    return true;
}

void FSteamAuthSession::PollAuthSessionStatus()
{
    if (!PollJobID) {
        Owner->PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
        auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsg.Header);
        header.set_jobid_source(Owner->GetNextJobID().GetValue());
        header.set_target_job_name("Authentication.PollAuthSessionStatus#1");
        Owner->PacketMsg.bProtoBuf = true;
        Owner->PacketMsg.MsgType = utilpp::steam::EMsg::ServiceMethodCallFromClientNonAuthed;
        utilpp::steam::CAuthentication_PollAuthSessionStatus_Request body;
        body.set_client_id(ClientID);
        body.set_request_id(RequestID);
        auto bodyLen = body.ByteSizeLong();
        auto [bufview, bres] = Owner->PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request::SerializeToOstream, &body, std::placeholders::_1));
        if (!bres) {
            return;
        }
        Owner->pWSClient->SendData(bufview.data(), bufview.size());
        PollJobID = Owner->PacketMsg.GetSourceJobID();
        Owner->JobManager.AddJob(PollJobID,
            std::bind(&FSteamAuthSession::OnPollAuthSessionStatusResponse, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&FSteamClient::OnRequestFinished, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
        );
    }
}

void FSteamAuthSession::OnGetPasswordRSAPublicKeyResponse(FSteamPacketMsg& msg, std::string_view bodyView)
{
    if (!AuthRequestHandlePtr->IsValid()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "OnGetPasswordRSAPublicKeyResponse while AuthRequestHandlePtr invalid");
        return;
    }
    auto bres = OnGetPasswordRSAPublicKeyResponseInternal(msg, bodyView);
    if (bres) {
        auto ExpectAuthSessionStatus = ESteamClientAuthSessionStatus::GettingPasswordRSAPublicKey;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, ESteamClientAuthSessionStatus::CreattingSeesion);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "GetPasswordRSAPublicKeyResponse failed");
        AuthSessionStatus = ESteamClientAuthSessionStatus::Unauthorized;
        return;
    }
    SIMPLELOG_LOGGER_DEBUG(nullptr, "GetPasswordRSAPublicKeyResponse success");
}

bool FSteamAuthSession::OnGetPasswordRSAPublicKeyResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView)
{
    bool bres{ true };
    int ires{ 1 };
    utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Response resp;
    bres = resp.ParseFromArray(bodyView.data(), bodyView.size());
    if (!bres) {
        return false;
    }
    auto& expStr = *resp.mutable_publickey_exp();
    auto& modStr = *resp.mutable_publickey_mod();
    FCharBuffer expBin(modStr.size() / 2);
    expBin.SetLength(expStr.size() / 2);
    FCharBuffer modBin(modStr.size() / 2);
    modBin.SetLength(modStr.size() / 2);
    if (!hex_to_bin((uint8_t*)expBin.Data(), expStr.data(), expStr.size())) {
        return false;
    }
    if (!hex_to_bin((uint8_t*)modBin.Data(), modStr.data(), modStr.size())) {
        return false;
    }
    FCryptoLibBinParams params;
    params.try_emplace("m", std::span<uint8_t>((uint8_t*)modBin.Data(), modBin.Length()));
    params.try_emplace("e", std::span<uint8_t>((uint8_t*)expBin.Data(), expBin.Length()));
    auto hkey = CryptoLibRSAGetKey(params);
    if (!hkey) {
        return false;
    }
    FCharBuffer encryptOut;
    if (!CryptoLibRSAEncrypt(hkey.get(), std::span<uint8_t>((uint8_t*)Password.data(), Password.size()), encryptOut)) {
        return false;
    }
    FCharBuffer base64Out;
    if (!CryptoLibBase64Encode(std::span<uint8_t>((uint8_t*)encryptOut.Data(), encryptOut.Length()), base64Out)) {
        return false;
    }
    Owner->PacketMsgInCB.Header = utilpp::steam::CMsgProtoBufHeader();
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsgInCB.Header);
    header.set_jobid_source(Owner->GetNextJobID().GetValue());
    header.set_target_job_name("Authentication.BeginAuthSessionViaCredentials#1");
    Owner->PacketMsgInCB.bProtoBuf = true;
    Owner->PacketMsgInCB.MsgType = utilpp::steam::EMsg::ServiceMethodCallFromClientNonAuthed;
    utilpp::steam::CAuthentication_BeginAuthSessionViaCredentials_Request body;
    body.set_account_name(AccountName);
    body.set_persistence(utilpp::steam::k_ESessionPersistence_Persistent);
    body.set_website_id("Client");
    body.set_encrypted_password(std::string_view(base64Out.Data(), base64Out.Length()));
    body.set_encryption_timestamp(resp.timestamp());
    utilpp::steam::CAuthentication_DeviceDetails* pDeviceDetails = new  utilpp::steam::CAuthentication_DeviceDetails;
    utilpp::steam::CAuthentication_DeviceDetails& DeviceDetails = *pDeviceDetails;
    DeviceDetails.set_device_friendly_name("SteamKit2");
    DeviceDetails.set_platform_type(utilpp::steam::EAuthTokenPlatformType::k_EAuthTokenPlatformType_SteamClient);
    DeviceDetails.set_os_type(std::to_underlying(Owner->OSType));
    body.set_allocated_device_details(pDeviceDetails);
    auto bodyLen = body.ByteSizeLong();
    auto tret = Owner->PacketMsgInCB.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_BeginAuthSessionViaCredentials_Request::SerializeToOstream, &body, std::placeholders::_1));
    auto& bufview = std::get<0>(tret);
    bres = std::get<1>(tret);
    if (!bres) {
        return false;
    }
    Owner->pWSClient->SendData(bufview.data(), bufview.size());
    auto SourceJobID = Owner->PacketMsgInCB.GetSourceJobID();
    AuthRequestHandlePtr->SourceJobID = SourceJobID;
    Owner->JobManager.AddJob(SourceJobID,
        std::bind(&FSteamAuthSession::OnBeginAuthSessionViaCredentialsResponse, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&FSteamClient::OnRequestFinished, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
    );
    return true;
}

void FSteamAuthSession::OnBeginAuthSessionViaCredentialsResponse(FSteamPacketMsg& msg, std::string_view bodyView)
{
    if (!AuthRequestHandlePtr->IsValid()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "OnBeginAuthSessionViaCredentialsResponse while AuthRequestHandlePtr invalid");
        return;
    }
    auto bres = OnBeginAuthSessionViaCredentialsResponseInternal(msg, bodyView);

    if (bres) {
        auto ExpectAuthSessionStatus = ESteamClientAuthSessionStatus::CreattingSeesion;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, ESteamClientAuthSessionStatus::PollingAuthSessionStatus);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "BeginAuthSessionViaCredentialsResponse failed");
        AuthSessionStatus = ESteamClientAuthSessionStatus::Unauthorized;
        Owner->OnRequestFinished(AuthRequestHandlePtr, SteamRequestFailedDelegate, ESteamClientError::SCE_RequestErrFromServer);
        return;
    }
    AuthRequestHandlePtr->SourceJobID = 0;
    SIMPLELOG_LOGGER_DEBUG(nullptr, "BeginAuthSessionViaCredentialsResponse seccess");

    std::transform(AllowedConfirmations.cbegin(), AllowedConfirmations.cend(),
        std::inserter(OutAllowedConfirmations, OutAllowedConfirmations.begin()),
        [](const std::pair<utilpp::steam::EAuthSessionGuardType, AllowedConfirmation_t>& key_value)
        {
            return ESteamClientAuthSessionGuardType(key_value.first);
        }
    );
    Owner->TriggerOnWaitConfirmationDelegates(OutAllowedConfirmations);
}

bool FSteamAuthSession::OnBeginAuthSessionViaCredentialsResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView)
{
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
    auto res = utilpp::steam::EResult(header.eresult());
    if (res != utilpp::steam::EResult::OK) {
        InvalidateCurrentAuth();
        return false;
    }
    utilpp::steam::CAuthentication_BeginAuthSessionViaCredentials_Response resp;
    auto bres = resp.ParseFromArray(bodyView.data(), bodyView.size());
    if (!bres) {
        return false;
    }
    ClientID = resp.client_id();
    Owner->SteamAccoutnInfo.SteamID = resp.steamid();
    RequestID = *resp.mutable_request_id();
    WeakToken = *resp.mutable_weak_token();
    auto& allowed_confirmations = *resp.mutable_allowed_confirmations();
    AllowedConfirmations.clear();
    for (int i = 0; i < allowed_confirmations.size(); i++) {
        AllowedConfirmations.try_emplace(allowed_confirmations[i].confirmation_type(), allowed_confirmations[i].confirmation_type(), *allowed_confirmations[i].mutable_associated_message());
    }
    return true;
}

void FSteamAuthSession::OnPollAuthSessionStatusResponse(FSteamPacketMsg& msg, std::string_view bodyView)
{
    if (!AuthRequestHandlePtr->IsValid()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "OnPollAuthSessionStatusResponse while AuthRequestHandlePtr invalid");
        return;
    }
    auto bres = OnPollAuthSessionStatusResponseInternal(msg, bodyView);
    PollJobID = 0;
    if (bres) {
        auto ExpectAuthSessionStatus = ESteamClientAuthSessionStatus::PollingAuthSessionStatus;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, ESteamClientAuthSessionStatus::Authenticated);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "PollAuthSessionStatusResponse failed");
        return;
    }
    if (!UpdateAccountCache()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "UpdateAccountCache failed");
        AuthSessionStatus = ESteamClientAuthSessionStatus::Error;
        return;
    }
    SIMPLELOG_LOGGER_DEBUG(nullptr, "PollAuthSessionStatusResponse success");
}

bool FSteamAuthSession::OnPollAuthSessionStatusResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView)
{
    auto& respheader = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
    if (utilpp::steam::EResult(respheader.eresult()) != utilpp::steam::EResult::OK) {
        return false;
    }
    utilpp::steam::CAuthentication_PollAuthSessionStatus_Response resp;
    auto bres = resp.ParseFromArray(bodyView.data(), bodyView.size());
    if (!bres) {
        return false;
    }
    RefreshToken = *resp.mutable_refresh_token();
    if (RefreshToken.size() <= 0) {
        return false;
    }
    ClientID = resp.new_client_id();
    AccessToken = *resp.mutable_access_token();
    AccountName = *resp.mutable_account_name();
    NewGuardData = *resp.mutable_new_guard_data();
    ChallengeUrl = *resp.mutable_new_challenge_url();
    AgreementSessionUrl = *resp.mutable_agreement_session_url();
    return true;
}

void FSteamAuthSession::OnUpdateAuthSessionWithSteamGuardCodeResponse(FSteamPacketMsg& msg, std::string_view bodyView)
{
    if (!SteamGuardCodeRequestHandlePtr->IsValid()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "OnUpdateAuthSessionWithSteamGuardCodeResponse while SteamGuardCodeRequestHandlePtr invalid");
        return;
    }
    bool bres{ false };
    FunctionExitHelper_t postExitHelper(
        [&]() {
            SteamGuardCodeRequestHandlePtr->bFinished = true;
            if (!bres) {
                SteamGuardCodeRequestHandlePtr->FinishCode = std::error_code(std::to_underlying(ESteamClientError::SCE_RequestErrFromServer), SteamClientErrorCategory());
            }
            else {
                PollAuthSessionStatus();
            }
            SteamGuardCodeDelegate(SteamGuardCodeRequestHandlePtr->FinishCode);
        }
    );
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
    auto res = utilpp::steam::EResult(header.eresult());
    if (res != utilpp::steam::EResult::OK && res != utilpp::steam::EResult::DuplicateRequest) {
        return;
    }
    utilpp::steam::CAuthentication_UpdateAuthSessionWithSteamGuardCode_Response resp;
    bres = resp.ParseFromArray(bodyView.data(), bodyView.size());
    if (!bres) {
        return;
    }

}

