#include "SteamMsg/SteamAuthSession.h"
#include "SteamMsg/SteamClientImpl.h"
#include <FunctionExitHelper.h>
#include <LoggerHelper.h>
#include <os_sock_helper.h>
#include <os_info_helper.h>
#include <SteamLanguageInternal.h>
#include <steam/steammessages_clientserver_login.pb.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/param_build.h>

bool FSteamAuthSession::OnSteamPackageReceived(FSteamPacketMsg& msg, std::string_view bodyView)
{
    return false;
}
void FSteamAuthSession::OnSteamCodeInput(std::string_view code)
{
}
void FSteamAuthSession::Tick(float delta)
{
    auto CurAuthSessionStatus = AuthSessionStatus.load();
    switch (CurAuthSessionStatus) {
    case EAuthSessionStatus::WaittingConnection: {
        if (Owner->LogStatus == ELogStatus::Logout) {
            Owner->PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
            auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsg.Header);
            header.set_jobid_source(Owner->GetNextJobID().GetValue());
            header.set_target_job_name("Authentication.GetPasswordRSAPublicKey#1");
            Owner->PacketMsg.bProtoBuf = true;
            Owner->PacketMsg.MsgType = EMsg::ServiceMethodCallFromClientNonAuthed;
            utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request body;
            body.set_account_name(Account);
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
                std::bind(&FSteamClient::OnRequestFailed, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
            );
            AuthSessionStatus = EAuthSessionStatus::GettingPasswordRSAPublicKey;
        }
        break;
    }
    case EAuthSessionStatus::PollingAuthSessionStatus: {
        if (bRequestSteamCode) {
            if (AllowedConfirmations.contains(utilpp::steam::k_EAuthSessionGuardType_DeviceCode)) {

            }
            if (AllowedConfirmations.contains(utilpp::steam::k_EAuthSessionGuardType_EmailCode)) {

            }
            bRequestSteamCode = false;
        }
        if (!PollJobID && AllowedConfirmations.contains(utilpp::steam::k_EAuthSessionGuardType_DeviceConfirmation)) {
            Owner->PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
            auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsg.Header);
            header.set_jobid_source(Owner->GetNextJobID().GetValue());
            header.set_target_job_name("Authentication.PollAuthSessionStatus#1");
            Owner->PacketMsg.bProtoBuf = true;
            Owner->PacketMsg.MsgType = EMsg::ServiceMethodCallFromClientNonAuthed;
            utilpp::steam::CAuthentication_PollAuthSessionStatus_Request body;
            body.set_client_id(ClientID);
            body.set_request_id(RequestID);
            auto bodyLen = body.ByteSizeLong();
            auto [bufview, bres] = Owner->PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_GetPasswordRSAPublicKey_Request::SerializeToOstream, &body, std::placeholders::_1));
            if (!bres) {
                break;
            }
            Owner->pWSClient->SendData(bufview.data(), bufview.size());
            PollJobID = Owner->PacketMsg.GetSourceJobID();
            Owner->JobManager.AddJob(PollJobID,
                std::bind(&FSteamAuthSession::OnPollAuthSessionStatusResponse, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&FSteamClient::OnRequestFailed, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
            );
        }
    }
    }
}
FCommonHandlePtr FSteamAuthSession::BeginAuthSessionViaCredentials(std::string_view account, std::string_view password, ISteamClient::FSteamRequestFailedDelegate FailedDelegate,std::error_code& ec)
{
    if (Account == account && Password == password) {
        if (AuthRequestHandlePtr) {
            return AuthRequestHandlePtr;
        }
        else {
            ec= std::error_code(std::to_underlying(ESteamClientError::SCE_AlreadyLoggedin), SteamClientErrorCategory());
            return nullptr;
        }
    }
    if (AuthRequestHandlePtr) {
        AuthRequestHandlePtr->bFinished = true;
    }
    Account = account;
    Password = password;
    SteamRequestFailedDelegate = FailedDelegate;
    AuthRequestHandlePtr = std::make_shared<SteamRequestHandle_t>();
    AuthRequestHandlePtr->bFinished = false;
    AuthSessionStatus = EAuthSessionStatus::WaittingConnection;
    return AuthRequestHandlePtr;
}

void FSteamAuthSession::OnGetPasswordRSAPublicKeyResponse(FSteamPacketMsg& msg, std::string_view bodyView)
{
    if (!AuthRequestHandlePtr->IsValid()) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "GotPasswordRSAPublicKey while AuthRequestHandlePtr invalid");
        return;
    }
    auto bres = OnGetPasswordRSAPublicKeyResponseInternal(msg, bodyView);
    if (bres) {
        auto ExpectAuthSessionStatus = EAuthSessionStatus::GettingPasswordRSAPublicKey;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, EAuthSessionStatus::CreattingSeesion);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "GetPasswordRSAPublicKeyResponse failed");
        AuthSessionStatus = EAuthSessionStatus::Unauthorized;
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

    RSA* rsa;
    size_t cipherlen;
    BIO* b64{ nullptr };
    uint8_t* cipher{ NULL };
    EVP_PKEY* key = NULL;
    EVP_PKEY_CTX* genctx = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    ENGINE* eng = NULL;
    auto pad = RSA_PKCS1_OAEP_PADDING;
    OSSL_PARAM_BLD* param_bld{ NULL };
    OSSL_PARAM* params = NULL;
    BIGNUM* n{ NULL }, * e{ NULL };
    FunctionExitHelper_t opensslHelper([&]() {
        if (params)
            OSSL_PARAM_free(params);
        if (param_bld)
            OSSL_PARAM_BLD_free(param_bld);
        if (ctx)
            EVP_PKEY_CTX_free(ctx);
        if (genctx)
            EVP_PKEY_CTX_free(genctx);

        if (key)
            EVP_PKEY_free(key);
        if (cipher)
            OPENSSL_free(cipher);
        if (b64) {
            BIO_pop(b64);
            BIO_free_all(b64);
        }
        if (rsa) {
            RSA_free(rsa);
        }
        else {
            if (n)
                BN_free(n);
            if (e)
                BN_free(e);
        }
        });
    ires = BN_hex2bn(&n, modStr.c_str());
    if (ires <= 0) {
        return false;
    }
    ires = BN_hex2bn(&e, expStr.c_str());
    if (ires <= 0) {
        return false;
    }
    //param_bld = OSSL_PARAM_BLD_new();
    //OSSL_PARAM_BLD_push_BN(param_bld, "n", n);
    //OSSL_PARAM_BLD_push_BN(param_bld, "e", e);
    //params = OSSL_PARAM_BLD_to_param(param_bld);
    //genctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    //if (!genctx)
    //    return false;
    //if (EVP_PKEY_fromdata_init(genctx) <= 0) {
    //    return false;
    //}
    //ires = EVP_PKEY_fromdata(genctx, &key, EVP_PKEY_PUBLIC_KEY, params);
    //if (ires <= 0) {
    //    return false;
    //}
    //ctx = EVP_PKEY_CTX_new(key, eng);
    //if (!ctx)
    //    return false;
    //ires = EVP_PKEY_encrypt_init(ctx);
    //if (ires <= 0)
    //    return false;
    ////if (EVP_PKEY_CTX_set_rsa_padding(ctx, pad) <= 0)
    //if (RSA_pkey_ctx_ctrl(ctx, -1, EVP_PKEY_CTRL_RSA_PADDING, pad, NULL) <= 0)
    //    return false;
    //if (EVP_PKEY_encrypt(ctx, NULL, &cipherlen, (uint8_t*)Password.data(), Password.size()) <= 0)
    //    return false;
    //cipher = (unsigned char*)OPENSSL_malloc(cipherlen);
    //if (!cipher)
    //    return false;
    //if (EVP_PKEY_encrypt(ctx, cipher, &cipherlen, (uint8_t*)Password.data(), Password.size()) <= 0)
    //    return false;

    cipher = (unsigned char*)OPENSSL_malloc(1024);
    rsa = RSA_new();
    RSA_set0_key(rsa, n, e, NULL);
    cipherlen = RSA_public_encrypt(Password.size(), (uint8_t*)Password.data(), cipher, rsa, RSA_PKCS1_PADDING);
    if (cipherlen == -1) {
        return false;
    }


    b64 = BIO_new(BIO_f_base64()); // create BIO to perform base64
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* mem = BIO_new(BIO_s_mem()); // create BIO that holds the result
    // chain base64 with mem, so writing to b64 will encode base64 and write to mem.
    BIO_push(b64, mem);
    // write data
    bool done = false;
    while (!done)
    {
        ires = BIO_write(b64, cipher, cipherlen);
        if (ires <= 0) // if failed
        {
            if (BIO_should_retry(b64)) {
                continue;
            }
            else // encoding failed
            {
                return false;
            }
        }
        else // success!
            done = true;
    }
    BIO_flush(b64);
    // get a pointer to mem's data
    unsigned char* base64Output;
    int base64Len;
    base64Len = BIO_get_mem_data(mem, &base64Output);



    Owner->PacketMsgInCB.Header = utilpp::steam::CMsgProtoBufHeader();
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(Owner->PacketMsgInCB.Header);
    header.set_jobid_source(Owner->GetNextJobID().GetValue());
    header.set_target_job_name("Authentication.BeginAuthSessionViaCredentials#1");
    Owner->PacketMsgInCB.bProtoBuf = true;
    Owner->PacketMsgInCB.MsgType = EMsg::ServiceMethodCallFromClientNonAuthed;
    utilpp::steam::CAuthentication_BeginAuthSessionViaCredentials_Request body;
    body.set_account_name(Account);
    body.set_persistence(utilpp::steam::k_ESessionPersistence_Ephemeral);
    body.set_website_id("Client");
    body.set_encrypted_password(std::string_view((char*)base64Output, base64Len));
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
        std::bind(&FSteamClient::OnRequestFailed, Owner, AuthRequestHandlePtr, SteamRequestFailedDelegate, std::placeholders::_1)
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
    bRequestSteamCode = true;
    if (bres) {
        auto ExpectAuthSessionStatus = EAuthSessionStatus::CreattingSeesion;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, EAuthSessionStatus::PollingAuthSessionStatus);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "BeginAuthSessionViaCredentialsResponse failed");
        AuthSessionStatus = EAuthSessionStatus::Unauthorized;
        Owner->OnRequestFailed(AuthRequestHandlePtr, SteamRequestFailedDelegate, ESteamClientError::SCE_RequestErrFromServer);
        return;
    }
    SIMPLELOG_LOGGER_DEBUG(nullptr, "BeginAuthSessionViaCredentialsResponse seccess");
}

bool FSteamAuthSession::OnBeginAuthSessionViaCredentialsResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView)
{
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
    auto res = EResult(header.eresult());
    if (res != EResult::OK) {
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
        auto ExpectAuthSessionStatus = EAuthSessionStatus::PollingAuthSessionStatus;
        bres = AuthSessionStatus.compare_exchange_strong(ExpectAuthSessionStatus, EAuthSessionStatus::Authenticated);
    }
    if (!bres) {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "PollAuthSessionStatusResponse failed");
        //AuthSessionStatus = EAuthSessionStatus::Unauthorized;
        //Owner->OnRequestFailed(AuthRequestHandlePtr, SteamRequestFailedDelegate, ESteamClientError::SCE_RequestErrFromServer);
        return;
    }
    AuthRequestHandlePtr.reset();
    SIMPLELOG_LOGGER_DEBUG(nullptr, "PollAuthSessionStatusResponse success");
}

bool FSteamAuthSession::OnPollAuthSessionStatusResponseInternal(FSteamPacketMsg& msg, std::string_view bodyView)
{
    auto& respheader = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
    if (EResult(respheader.eresult()) != EResult::OK) {
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

