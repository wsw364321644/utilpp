#include "SteamMsg/SteamClientImpl.h"
#include "SteamMsg/SteamClientInternal.h"

#include <steam_web_api_util.h>
#include <FunctionExitHelper.h>
#include <SteamLanguage.h>
#include <SteamLanguageInternal.h>
#include <hex.h>
#include <os_info_helper.h>
#include <string_convert.h>
#include <os_sock_helper.h>


#include <steam/steammessages_clientserver_login.pb.h>
#include <steam/steammessages_clientserver.pb.h>
#include <steam/steammessages_store.steamclient.pb.h>




FSteamClient::FSteamClient() :SteamAuthSession(this)
{
    JobID.SetStartTime(std::chrono::system_clock::now());
    OSInfo_t OSInfo;
    GetOsInfo(&OSInfo);
    OSType = utilpp::steam::EOSType(OSInfo.OSCoreType);

}

FSteamClient::~FSteamClient()
{
    //if (pSelectAccountPSO) {
    //    sqlite3_finalize(pSelectAccountPSO);
    //}

    //if (pSQLite) {
    //    sqlite3_close(pSQLite);
    //}
}

bool FSteamClient::Init(IWebsocketConnectionManager* _pWebsocketConnectionManager, HttpManagerPtr _pHttpManager)
{
    if (!_pWebsocketConnectionManager || !_pHttpManager) {
        return false;
    }
    pWSManager = _pWebsocketConnectionManager;
    pHttpManager = _pHttpManager;

    pWSClient = pWSManager->CreateClient();
    pWSClient->AddOnConnectedDelegate(
        [&](std::shared_ptr<IWebsocketClient> pClient) {
            std::error_code ec;
            ClientHello(ec);
            if (ec) {
                Disconnect();
                LogStatus = ELogStatus::NotConnect;
            }
            else {
                LogStatus = ELogStatus::Logout;
                TriggerOnConnectedDelegates();
            }
        }
    );
    pWSClient->AddOnReceivedDelegate(std::bind(&FSteamClient::OnWSDataReceived, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    pWSClient->AddOnDisconnectedDelegate(
        [&](const std::shared_ptr<IWebsocketClient>&, const std::error_code& ec) {
            LogStatus = ELogStatus::NotConnect;
            TriggerOnDisconnectedDelegates(ec);
        }
    );
    //auto ires= sqlite3_open_v2(SQL_FILE_NAME, &pSQLite, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //char* zErrMsg = 0;
    //ires = sqlite3_exec(pSQLite, SQL_CREATE_STEAM_USER_TABLE, SqliteCB, 0, &zErrMsg);
    //if (ires != SQLITE_OK) {
    //    auto msg = sqlite3_errmsg(pSQLite);
    //    return false;
    //}
    //auto selectSql=std::format(SQL_SELECT_FROM_STEAM_USER_BY_ACCOUNTNAME, "RefreshToken");
    //ires = sqlite3_prepare_v2(pSQLite, selectSql.c_str(), -1, &pSelectAccountPSO, 0);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    //ires = sqlite3_prepare_v2(pSQLite, SQL_INSERT_STEAM_USER, -1, &pInsertAccountPSO, 0);
    //if (ires != SQLITE_OK) {
    //    return false;
    //}
    auto config = std::make_shared<sqlpp::sqlite3::connection_config>();
    config->path_to_database = SQL_FILE_NAME;
    config->flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    DBConnectionPool.initialize(config, 10);
    return true;
}



void FSteamClient::Disconnect()
{
    pWSManager->Disconnect(pWSClient);
}

void FSteamClient::CancelRequest(FCommonHandlePtr CommonHandlePtr)
{
}

FCommonHandlePtr FSteamClient::Login(std::string_view account, std::string_view password, FSteamRequestFailedDelegate FailedDelegate,std::error_code& ec)
{
    return SteamAuthSession.BeginAuthSessionViaCredentials(account, password, FailedDelegate, ec);
}

void FSteamClient::InputSteamGuardCode(std::string_view codeView)
{
    SteamAuthSession.InputSteamGuardCode(codeView);
}

FCommonHandlePtr FSteamClient::RegisterKey(std::string_view keyView, FSteamRequestFinishedDelegate Delegate, std::error_code& ec)
{
    if (LogStatus!= ELogStatus::Logon) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_NotLogin), SteamClientErrorCategory());
    }

    PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
    PacketMsg.bProtoBuf = true;
    PacketMsg.MsgType = utilpp::steam::EMsg::ServiceMethodCallFromClient;
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(PacketMsg.Header);
    header.set_jobid_source(GetNextJobID().GetValue());
    header.set_target_job_name("Store.RegisterCDKey#1");
    header.set_client_sessionid(SessionID);
    header.set_steamid(SteamAccoutnInfo.SteamID);

    utilpp::steam::CStore_RegisterCDKey_Request body;
    body.set_activation_code(keyView);
    body.set_is_request_from_client(true);
    auto bodyLen = body.ByteSizeLong();



    auto [bufview, bres] = PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CStore_RegisterCDKey_Request::SerializeToOstream, &body, std::placeholders::_1));
    if (!bres) {
        return nullptr;
    }
    pWSClient->SendData(bufview.data(), bufview.size());
    auto SourceJobID = PacketMsg.GetSourceJobID();
    auto RequestHandlePtr = std::make_shared<SteamRequestHandle_t>(SourceJobID);
    JobManager.AddJob(SourceJobID,
        [&, RequestHandlePtr, Delegate](FSteamPacketMsg& msg, std::string_view bodyView) {
            utilpp::steam::CStore_RegisterCDKey_Response body;
            auto bres=body.ParseFromArray(bodyView.data(), bodyView.size());
            if (!bres) {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_RequestErrFromServer);
                return;
            }
            auto result=(utilpp::steam::EResult)body.mutable_purchase_receipt_info()->purchase_status();
            auto purchaseResultDetail = (utilpp::steam::EPurchaseResultDetail)body.purchase_result_details();
            switch (purchaseResultDetail) {
            case utilpp::steam::EPurchaseResultDetail::AccountLocked: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_RequestErrFromServer);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::AlreadyPurchased: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_AlreadyPurchased);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::CannotRedeemCodeFromClient: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_CannotRedeemCodeFromClient);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::DoesNotOwnRequiredApp: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_DoesNotOwnRequiredApp);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::NoWallet: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_NoWallet);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::RestrictedCountry: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_RestrictedCountry);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::Timeout: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_RequestTimeout);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::BadActivationCode: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_BadActivationCode);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::DuplicateActivationCode: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_DuplicateActivationCode);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::NoDetail: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_OK);
                break;
            }
            case utilpp::steam::EPurchaseResultDetail::RateLimited: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_RateLimited);
                break;
            }
            default: {
                OnRequestFinished(RequestHandlePtr, Delegate, ESteamClientError::SCE_UnknowError);
                break;
            }
            }
            RequestHandlePtr->bFinished = true;
        },
        std::bind(&FSteamClient::OnRequestFinished, this, RequestHandlePtr, Delegate, std::placeholders::_1)
    );
    return RequestHandlePtr;
}

void FSteamClient::Tick(float delta)
{
    static auto PreLogStatus = ELogStatus::NotConnect;
    switch (LogStatus) {
    case ELogStatus::NotConnect: {
        if (SteamAuthSession.AuthSessionStatus != EAuthSessionStatus::Unauthorized) {
            if (SteamAuthSession.AuthSessionStatus != EAuthSessionStatus::Authenticated) {
                SteamAuthSession.AuthSessionStatus = EAuthSessionStatus::WaittingConnection;
            }
            if (Connect()) {
                LogStatus = ELogStatus::Connecting;
            }
            else {
                LogStatus = ELogStatus::Error;
            }
        }
        break;
    }
    case ELogStatus::Logout: {
        if (SteamAuthSession.AuthSessionStatus == EAuthSessionStatus::Authenticated) {
            if (Logon()) {
                LogStatus = ELogStatus::Loggingon;
            }
            else {
                LogStatus = ELogStatus::Error;
            }
        }
        break;
    }
    case ELogStatus::Logon: {
        if (PreLogStatus != ELogStatus::Logon) {
            std::error_code ec;
            HeartBeat(ec);
            TriggerOnLoginDelegates(SteamAccoutnInfo);
        }
        HeartBeatSecCount += delta;
        if (HeartBeatSecCount > HeartBeatSec) {
            HeartBeatSecCount = 0;
            std::error_code ec;
            HeartBeat(ec);
        }
        break;
    }
    }
    PreLogStatus = LogStatus;
    SteamAuthSession.Tick(delta);
}

void FSteamClient::ClientHello(std::error_code& ec)
{
    if (!pWSClient->IsConnected()) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_NotConnected), SteamClientErrorCategory());
    }

    utilpp::steam::CMsgClientHello body;
    body.set_protocol_version(MsgClientLogon::CurrentProtocol);
    auto bodyLen = body.ByteSizeLong();
    PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
    PacketMsg.bProtoBuf = true;
    PacketMsg.MsgType = utilpp::steam::EMsg::ClientHello;
    auto [bufview, bres] = PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CMsgClientHello::SerializeToOstream, &body, std::placeholders::_1));
    if (!bres) {
        return;
    }
    pWSClient->SendData(bufview.data(), bufview.size());
}

void FSteamClient::HeartBeat(std::error_code& ec)
{
    if (!pWSClient->IsConnected()) {
        ec = std::error_code(std::to_underlying(ESteamClientError::SCE_NotConnected), SteamClientErrorCategory());
    }

    utilpp::steam::CMsgClientHeartBeat body;
    auto bodyLen = body.ByteSizeLong();
    PacketMsg.Header = utilpp::steam::CMsgProtoBufHeader();
    PacketMsg.bProtoBuf = true;
    PacketMsg.MsgType = utilpp::steam::EMsg::ClientHeartBeat;
    auto [bufview, bres] = PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CMsgClientHello::SerializeToOstream, &body, std::placeholders::_1));
    if (!bres) {
        return;
    }
    pWSClient->SendData(bufview.data(), bufview.size());
}

sqlpp::sqlite3::pooled_connection& FSteamClient::GetDBConnection()
{
    if (!DBConnection.has_value()) {
        DBConnection = DBConnectionPool.get();
    }
    return DBConnection.value();
}

bool FSteamClient::Connect()
{
    auto req=utilpp::GetCMListForConnect(pHttpManager,
        [&](bool bres, std::u8string_view body) {
            if (!bres) {
                LogStatus = ELogStatus::NotConnect;
                return;
            }
            utilpp::GetCMListForConnectResp_t resp;
            if (!resp.Parse(body)) {
                LogStatus = ELogStatus::NotConnect;
                return;
            }

            pWSClient->SetHost(ConvertStringToU8View(resp.EndpointDetails[0].Host));
            pWSClient->SetPath(u8"/cmsocket/");
            pWSClient->SetPortNum(resp.EndpointDetails[0].Port);
            pWSClient->SetSSl(true);
            pWSManager->Connect(pWSClient);
        }
    );
    if (!req) {
        return false;
    }
    return true;
}

bool FSteamClient::Logon()
{
    PacketMsgInCB.Header = utilpp::steam::CMsgProtoBufHeader();
    auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(PacketMsgInCB.Header);
    header.set_client_sessionid(0);
    steamIDSetting.SetAccountInstance(FSteamID::DesktopInstance);
    steamIDSetting.SetAccountUniverse(utilpp::steam::EUniverse::Public);
    steamIDSetting.SetAccountType(utilpp::steam::EAccountType::Individual);
    header.set_steamid(steamIDSetting.GetValue());
    PacketMsgInCB.bProtoBuf = true;
    PacketMsgInCB.MsgType = utilpp::steam::EMsg::ClientLogon;

    utilpp::steam::CMsgClientLogon body;
    auto pMsgIPAddress = new  utilpp::steam::CMsgIPAddress;
    auto& MsgIPAddress = *pMsgIPAddress;
    MsgIPAddress.set_v4(0xFFFFFFFF ^ MsgClientLogon::ObfuscationMask);
    body.set_allocated_obfuscated_private_ip(pMsgIPAddress);
    if (body.mutable_obfuscated_private_ip()->has_v4()) {
        body.set_deprecated_obfustucated_private_ip(MsgIPAddress.v4());
    }

    body.set_account_name(SteamAuthSession.AccountName);
    body.clear_password();
    body.set_access_token(SteamAuthSession.RefreshToken);
    body.set_should_remember_password(false);

    body.set_protocol_version(MsgClientLogon::CurrentProtocol);
    body.set_client_os_type(std::to_underlying(OSType));
    body.set_client_language("english");
    body.set_cell_id(0);
    body.set_steam2_ticket_request(false);

    body.set_client_package_version(1771);
    body.set_supports_rate_limit_response(true);
    char name[HOST_NAME_MAX];
    gethostname(name, sizeof(name));
    body.set_machine_name(name);
    char MachineID[MACHINE_GUID_MAX + 1];
    auto MachineIDLen = sizeof(MachineID);
    bool bGUID;
    GetMachineUUID(MachineID, &MachineIDLen, &bGUID);
    body.set_machine_id(MachineID);

    auto bodyLen = body.ByteSizeLong();
    auto tret = PacketMsgInCB.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CAuthentication_BeginAuthSessionViaCredentials_Request::SerializeToOstream, &body, std::placeholders::_1));
    auto& bufview = std::get<0>(tret);
    auto bres = std::get<1>(tret);
    if (!bres) {
        return false;
    }
    pWSClient->SendData(bufview.data(), bufview.size());
    return true;
}

void FSteamClient::OnRequestFinished(std::shared_ptr<SteamRequestHandle_t> SteamRequestHandle, FSteamRequestFinishedDelegate SteamRequestFailedDelegate, ESteamClientError err)
{
    SteamRequestHandle->bFinished = true;
    SteamRequestFailedDelegate(std::error_code(std::to_underlying(err), SteamClientErrorCategory()));
}

void FSteamClient::OnWSDataReceived(const std::shared_ptr<IWebsocketClient>& pWSClient, const char* content, size_t len)
{
    static FSteamPacketMsg msg;
    auto [bodyView, bres] = msg.Parse(content, len);
    if (!bres) {
        return;
    }
    if (SteamAuthSession.OnSteamPackageReceived(msg, bodyView)) {
        return;
    }
    switch (msg.GetMsgType()) {
    case utilpp::steam::EMsg::Multi: {
        if (!msg.IsProtoBuf()) {
            return;
        }
        auto bres = MsgMulti.ParseFromArray(bodyView.data(), bodyView.size());
        if (!bres) {
            return;
        }
        auto pMessageBodyStr = MsgMulti.mutable_message_body();
        std::string_view pMessagebodyView;
        if (MsgMulti.size_unzipped() > 0) {
            UncompressMessageBodyBuf.reserve(MsgMulti.size_unzipped());
            ZS.zalloc = Z_NULL;
            ZS.zfree = Z_NULL;
            ZS.opaque = Z_NULL;
            ZS.next_in = (Bytef*)pMessageBodyStr->data();
            ZS.avail_in = pMessageBodyStr->size();
            ZS.total_out = 0;
            ZS.next_out = (Bytef*)UncompressMessageBodyBuf.data();
            ZS.avail_out = MsgMulti.size_unzipped();
            if (inflateInit2(&ZS, (16 + MAX_WBITS)) != Z_OK) {
                return;
            }
            int err = inflate(&ZS, Z_NO_FLUSH);
            if (err < 0) {
                return;
            }
            if (inflateEnd(&ZS) != Z_OK) {
                return;
            }
            if (ZS.total_out != MsgMulti.size_unzipped()) {
                return;
            }
            pMessagebodyView = std::string_view((char*)UncompressMessageBodyBuf.data(), MsgMulti.size_unzipped());
        }
        else {
            pMessagebodyView = *pMessageBodyStr;
        }
        const char* cursor = pMessagebodyView.data();
        while (cursor < pMessagebodyView.data() + pMessagebodyView.size()) {
            auto& length = *(int32_t*)cursor;
            cursor += sizeof(decltype(length));
            if (cursor + length > pMessagebodyView.data() + pMessagebodyView.size()) {
                break;
            }
            OnWSDataReceived(pWSClient, cursor, length);
            cursor += length;
        }
    }
    case utilpp::steam::EMsg::ClientServerUnavailable: {
        //todo MsgClientServerUnavailable
        break;
    }
    case utilpp::steam::EMsg::ClientSessionToken: { // am session token
        utilpp::steam::CMsgClientSessionToken body;
        auto bres = body.ParseFromArray(bodyView.data(), bodyView.size());
        if (!bres) {
            return;
        }
        SessionToken = body.token();
        break;
    }
    case utilpp::steam::EMsg::JobHeartbeat: {
        JobManager.JobHeartBeat(msg.GetTargetJobID());
        break;
    }
    case utilpp::steam::EMsg::DestJobFailed: {
        JobManager.JobFailed(msg.GetTargetJobID());
        break;
    }
    case utilpp::steam::EMsg::ServiceMethodResponse: {
        JobManager.JobCompleted(msg, bodyView);
        break;
    }
    case utilpp::steam::EMsg::ServiceMethod: {
        //todo notification
        break;
    }
    case utilpp::steam::EMsg::ClientLogOnResponse: { // we handle this to get the SteamID/SessionID and to setup heartbeating
        if (!msg.IsProtoBuf()) {
            LogStatus = ELogStatus::Error;
            return;
        }
        auto& header = std::get<utilpp::steam::CMsgProtoBufHeader>(msg.Header);
        utilpp::steam::CMsgClientLogonResponse MsgClientLogonResponse;
        auto bres = MsgClientLogonResponse.ParseFromArray(bodyView.data(), bodyView.size());
        if (!bres) {
            LogStatus = ELogStatus::Error;
            return;
        }
        auto res = utilpp::steam::EResult(MsgClientLogonResponse.eresult());
        if (res == utilpp::steam::EResult::OK) {
            SessionID = header.client_sessionid();
            SteamAccoutnInfo.SteamID = header.steamid();
            CellID = MsgClientLogonResponse.cell_id();
            IPCountryCode = *MsgClientLogonResponse.mutable_ip_country_code();
            if (MsgClientLogonResponse.mutable_public_ip()->has_v6()) {
                PublicIP = MsgClientLogonResponse.mutable_public_ip()->v6();
            }
            else {
                PublicIP = MsgClientLogonResponse.mutable_public_ip()->v4();
            }
            HeartBeatSec = MsgClientLogonResponse.heartbeat_seconds();
            HeartBeatSecCount = 0;
            LogStatus = ELogStatus::Logon;
        }
        else if (res == utilpp::steam::EResult::TryAnotherCM || res == utilpp::steam::EResult::ServiceUnavailable) {
            LogStatus = ELogStatus::NotConnect;
        }
        break;
    }
    case utilpp::steam::EMsg::ClientLoggedOff: { // to stop heartbeating when we get logged off
        LogStatus = ELogStatus::Logout;
        SessionID = 0;
        SteamAccoutnInfo.SteamID = 0;
        CellID = 0;
        PublicIP = 0u;
        IPCountryCode.clear();
        if (msg.IsProtoBuf())
        {
            utilpp::steam::CMsgClientLoggedOff body;
            auto bres = body.ParseFromArray(bodyView.data(), bodyView.size());
            if (!bres) {
                LogStatus = ELogStatus::Error;
                return;
            }
            auto logoffResult = utilpp::steam::EResult(body.eresult());
            if (logoffResult == utilpp::steam::EResult::TryAnotherCM || logoffResult == utilpp::steam::EResult::ServiceUnavailable)
            {
                LogStatus = ELogStatus::NotConnect;
            }
        }
        std::error_code ec;
        TriggerOnLogoutDelegates(ec);
        break;
    }
    }

}

ISteamClient* GetSteamClientSingleton() {
    static std::atomic<std::shared_ptr<FSteamClient>> AtomicPtr;
    auto oldptr = AtomicPtr.load();
    if (!oldptr) {
        std::shared_ptr<FSteamClient> pTaskManager(new FSteamClient);
        AtomicPtr.compare_exchange_strong(oldptr, pTaskManager);
    }
    return AtomicPtr.load().get();
}