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




FSteamClient::FSteamClient() :SteamAuthSession(this)
{
    JobID.SetStartTime(std::chrono::system_clock::now());
    OSInfo_t OSInfo;
    GetOsInfo(&OSInfo);
    OSType = EOSType(OSInfo.OSCoreType);
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

void FSteamClient::Tick(float delta)
{
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
        HeartBeatSecCount += delta;
        if (HeartBeatSecCount > HeartBeatSec) {
            HeartBeatSecCount = 0;
            std::error_code ec;
            HeartBeat(ec);
        }
        break;
    }
    }
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
    PacketMsg.MsgType = EMsg::ClientHello;
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
    PacketMsg.MsgType = EMsg::ClientHeartBeat;
    auto [bufview, bres] = PacketMsg.SerializeToOstream(bodyLen, std::bind(&utilpp::steam::CMsgClientHello::SerializeToOstream, &body, std::placeholders::_1));
    if (!bres) {
        return;
    }
    pWSClient->SendData(bufview.data(), bufview.size());
}

bool FSteamClient::Connect()
{
    auto req=utilpp::GetCMListForConnect(pHttpManager,
        [&](bool bres, std::u8string_view body) {
            if (!bres) {
                LogStatus = ELogStatus::NotConnect;
            }
            utilpp::GetCMListForConnectResp_t resp;
            if (!resp.Parse(body)) {
                LogStatus = ELogStatus::NotConnect;
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
    steamIDSetting.SetAccountUniverse(EUniverse::Public);
    steamIDSetting.SetAccountType(EAccountType::Individual);
    header.set_steamid(steamIDSetting.GetValue());
    PacketMsgInCB.bProtoBuf = true;
    PacketMsgInCB.MsgType = EMsg::ClientLogon;

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

void FSteamClient::OnRequestFailed(std::shared_ptr<SteamRequestHandle_t> SteamRequestHandle, FSteamRequestFailedDelegate SteamRequestFailedDelegate, ESteamClientError err)
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
    case EMsg::Multi: {
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
    case EMsg::ClientServerUnavailable: {
        //todo MsgClientServerUnavailable
        break;
    }
    case EMsg::ClientSessionToken: { // am session token
        utilpp::steam::CMsgClientSessionToken body;
        auto bres = body.ParseFromArray(bodyView.data(), bodyView.size());
        if (!bres) {
            return;
        }
        SessionToken = body.token();
        break;
    }
    case EMsg::JobHeartbeat: {
        JobManager.JobHeartBeat(msg.GetTargetJobID());
        break;
    }
    case EMsg::DestJobFailed: {
        JobManager.JobFailed(msg.GetTargetJobID());
        break;
    }
    case EMsg::ServiceMethodResponse: {
        JobManager.JobCompleted(msg, bodyView);
        break;
    }
    case EMsg::ServiceMethod: {
        //todo notification
        break;
    }
    case EMsg::ClientLogOnResponse: { // we handle this to get the SteamID/SessionID and to setup heartbeating
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
        auto res = EResult(MsgClientLogonResponse.eresult());
        if (res == EResult::OK) {
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
            TriggerOnLoginDelegates(SteamAccoutnInfo);
        }
        else if (res == EResult::TryAnotherCM || res == EResult::ServiceUnavailable) {
            LogStatus = ELogStatus::NotConnect;
        }
        break;
    }
    case EMsg::ClientLoggedOff: { // to stop heartbeating when we get logged off
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
            auto logoffResult = EResult(body.eresult());
            if (logoffResult == EResult::TryAnotherCM || logoffResult == EResult::ServiceUnavailable)
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