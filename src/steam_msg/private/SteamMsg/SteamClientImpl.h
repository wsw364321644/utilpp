#pragma once
#include <string>
#include <zlib.h>
#include <zconf.h>
#include <concurrentqueue.h>

#include <TimeRecorder.h>
#include "SteamMsg/SteamPacketMessage.h"
#include "SteamMsg/SteamClientInternal.h"
#include "SteamMsg/SteamJobManager.h"
#include "SteamMsg/SteamAuthSession.h"
#include "SteamMsg/STEAMUSER.h"

class FSteamClient :public ISteamClient {
public:
    friend class FSteamAuthSession;
    FSteamClient();
    ~FSteamClient();
    bool Init(IWebsocketConnectionManager*, HttpManagerPtr) override;
    void Disconnect() override;
    void CancelRequest(FCommonHandlePtr) override;
    ESteamClientLogStatus GetLoginStatus() const override;
    ESteamClientAuthSessionStatus GetAuthSessionStatus()const override;
    const SteamAccoutnInfo_t& GetAccoutnInfo()const override;
    const std::unordered_set<ESteamClientAuthSessionGuardType>& GetAllowedConfirmations() const override;

    FCommonHandlePtr Login(std::string_view, std::string_view, FSteamRequestFinishedDelegate, std::error_code&) override;
    FCommonHandlePtr SendSteamGuardCode(std::string_view, FSteamRequestFinishedDelegate, std::error_code&) override;
    FCommonHandlePtr RegisterKey(std::string_view, FSteamRequestFinishedDelegate, std::error_code&) override;
    void Tick(float delta) override;

    void ClientHello(std::error_code&);
    void HeartBeat(std::error_code&);

    sqlpp::sqlite3::pooled_connection& GetDBConnection();
private:
    void Reconnect(bool bNeedDelay =false);
    bool Connect();
    bool Logon();
    void OnRequestFinished(std::shared_ptr<SteamRequestHandle_t>, FSteamRequestFinishedDelegate, ESteamClientError);
    void OnWSDataReceived(const std::shared_ptr<IWebsocketClient>& pWSClient, const char* content, size_t len);

    FSteamGlobalID& GetNextJobID() {
        JobID.SetSequentialCount(++SequentialCount);
        return JobID;
    }
    std::atomic< ESteamClientLogStatus> LogStatus{ ESteamClientLogStatus::NotConnect };
    HttpManagerPtr pHttpManager;
    IWebsocketConnectionManager* pWSManager{nullptr};
    std::shared_ptr<IWebsocketClient> pWSClient;
    sqlpp::sqlite3::connection_pool DBConnectionPool;
    inline static thread_local std::optional<sqlpp::sqlite3::pooled_connection> DBConnection;
    utilpp::steam::STEAMUSER SteamUserTable;
    bool bShouldReconnect{ false };
    FDelayRecorder ReconnectDelayRecorder;
    //count
    FSteamGlobalID JobID;
    uint32_t SequentialCount{ 0 };
    float HeartBeatSecCount{ 0 };

    std::vector<uint8_t> Modulus;
    std::vector<uint8_t> Exponent;
    int64_t SessionToken{ 0 };
    FSteamJobManager JobManager{};
    FSteamAuthSession SteamAuthSession;
    utilpp::steam::EOSType OSType;

    SteamAccoutnInfo_t SteamAccoutnInfo;
    //from log on
    int32_t SessionID{ 0 };
    uint32_t CellID{ 0 };
    std::variant<uint32_t, std::string>PublicIP{ 0u };
    std::string IPCountryCode;
    int32_t HeartBeatSec{ 0 };

    ////cache
    FSteamID steamIDSetting;
    FSteamPacketMsg PacketMsg;
    FSteamPacketMsg PacketMsgInCB;
    std::vector<uint8_t> UncompressMessageBodyBuf;
    utilpp::steam::CMsgMulti MsgMulti;
    z_stream ZS{};

    std::shared_ptr<SteamRequestHandle_t> GetNewRequestHandle() {
        std::shared_ptr<SteamRequestHandle_t> ptr;
        if(!RequestPool.try_dequeue(ptr)) {
            ptr = std::make_shared<SteamRequestHandle_t>();
        }
        ptr->bFinished = false;
        return ptr;
    }
    moodycamel::ConcurrentQueue<std::shared_ptr<SteamRequestHandle_t>> RequestPool;
    moodycamel::ConcurrentQueue<std::shared_ptr<SteamRequestHandle_t>> FinishedRequests;

};