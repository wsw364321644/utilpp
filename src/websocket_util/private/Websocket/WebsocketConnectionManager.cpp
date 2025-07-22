#include "Websocket/IWebsocketConnectionManager.h"
#include "ws_error.h"

#include <LoggerHelper.h>
#include <std_ext.h>
#include <libwebsockets.h>
#include <string_convert.h>

#include <unordered_set>
#include <array>
#include <concurrentqueue.h>
class FWebsocketConnectionManagerLWS;
constexpr char LWS_MANAGER_NAME[] = "LWS";
static TNamedClassAutoRegister_t<FWebsocketConnectionManagerLWS> NamedClassAutoRegister(LWS_MANAGER_NAME);

enum class ELWSStatus {
    LWS_None,
    LWS_Idle,
    LWS_Connecting,
    LWS_Connected,
    LWS_Error,
};
class FWebsocketEndpointLWS :public IWebsocketClient, public std::enable_shared_from_this<FWebsocketEndpointLWS> {
public:
    bool IsConnected()override {
        return Status== ELWSStatus::LWS_Connected;
    }
    void SetHost(std::u8string_view view) override {
        Host = ConvertU8ViewToString(view);
        LWSConnectInfo.address = Host.c_str();
        LWSConnectInfo.host = Host.c_str();
        LWSConnectInfo.origin = Host.c_str();
    }
    void SetPath(std::u8string_view view) override {
        Path = ConvertU8ViewToString(view);
        LWSConnectInfo.path = Path.c_str();
    }

    std::variant<uint32_t, std::string> GetLocalIP() override {
        return LocalIp;
    }

    void SetSSl(bool bssl)override {
        if (bssl) {
            LWSConnectInfo.ssl_connection |= LCCSCF_USE_SSL;
        }
        else {
            LWSConnectInfo.ssl_connection &= ~LCCSCF_USE_SSL;
        }
    }
    void SetPortNum(int inPort)override {
        LWSConnectInfo.port = inPort;
    }
    void SendData(const char* content, size_t len)override {
        std::shared_ptr<std::vector<uint8_t>> pSendBuf;
        if (!SendBufPool.try_dequeue(pSendBuf)) {
            pSendBuf = std::make_shared<std::vector<uint8_t>>();
        }
        pSendBuf->clear();
        pSendBuf->insert(pSendBuf->begin(), LWS_PRE, 0);
        pSendBuf->append_range(std::string_view(content, len));

        SendQueue.enqueue(pSendBuf);
        if (WSI) {
            lws_callback_on_writable(WSI);
        }
    }
    const std::error_code& GetLastError() const override {
        return LastError;
    }

    void SetError(std::error_code ec) {
        WSI = NULL;
        Status = ELWSStatus::LWS_Error;
        LastError = ec;
        auto pEndpoint = this->shared_from_this();
        pEndpoint->TriggerOnDisconnectedDelegates(pEndpoint, pEndpoint->GetLastError());
    }
    ~FWebsocketEndpointLWS() {
        if (LWSContext) {
            lws_context_destroy(LWSContext);
            LWSContext = nullptr;
        }
        Status = ELWSStatus::LWS_None;
    }
    void Clear() {
        memset(&LWSInfo, 0, sizeof LWSInfo);
    }

    std::string Host;
    std::string Path;
    FWebsocketConnectionManagerLWS* Owner{ nullptr };

    struct lws_context_creation_info LWSInfo { 0 };
    struct lws_context* LWSContext{ nullptr };
    struct lws_client_connect_info LWSConnectInfo { 0 };
    struct lws* WSI{ nullptr };
    ELWSStatus Status{ ELWSStatus::LWS_None };
    std::error_code LastError;
    std::variant<uint32_t, std::string> LocalIp;
    moodycamel::ConcurrentQueue<std::shared_ptr<std::vector<uint8_t>>> SendQueue;
    moodycamel::ConcurrentQueue<std::shared_ptr<std::vector<uint8_t>>> SendBufPool;
};

class FWebsocketConnectionManagerLWS : public IWebsocketConnectionManager {
public:
    std::shared_ptr<IWebsocketClient> CreateClient() override;
    void Connect(std::shared_ptr<IWebsocketClient>) override;
    void Disconnect(std::shared_ptr<IWebsocketClient>) override;
    void Tick(float delta) override;
private:
    static int LWSCallback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len);
    void RecycleWebsocketEndpointLWS(std::shared_ptr<FWebsocketEndpointLWS>);
    moodycamel::ConcurrentQueue<std::shared_ptr<FWebsocketEndpointLWS>> EndpiontPool;
    std::unordered_set<std::shared_ptr<FWebsocketEndpointLWS>> Endpionts;

    static inline const struct lws_protocols protocols[] = {
    { "lws_client", FWebsocketConnectionManagerLWS::LWSCallback, 0, 0, 0, NULL, 0 },
    LWS_PROTOCOL_LIST_TERM
    };
};



std::shared_ptr<IWebsocketClient> FWebsocketConnectionManagerLWS::CreateClient()
{
    std::shared_ptr<FWebsocketEndpointLWS> pEndpoint;
    EndpiontPool.try_dequeue(pEndpoint);
    if (!pEndpoint) {
        pEndpoint = std::make_shared<FWebsocketEndpointLWS>();
        pEndpoint->Owner = this;
        pEndpoint->LWSConnectInfo.userdata = pEndpoint.get();
        pEndpoint->LWSConnectInfo.pwsi = &pEndpoint->WSI;
    }
    Endpionts.emplace(pEndpoint);
    return pEndpoint;
}

void FWebsocketConnectionManagerLWS::Connect(std::shared_ptr<IWebsocketClient> pClient)
{
    auto pEndpoint = std::dynamic_pointer_cast<FWebsocketEndpointLWS>(pClient);
    struct lws_context_creation_info& info = pEndpoint->LWSInfo;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    info.fd_limit_per_thread = 1 + 1 + 1;
    pEndpoint->LWSContext = lws_create_context(&info);
    pEndpoint->LWSConnectInfo.context = pEndpoint->LWSContext;
    if (!pEndpoint->LWSContext) {
        pEndpoint->SetError(std::error_code(std::to_underlying(EWSError::WSE_INTERNEL_WSLIB_ERROR), ws_category()));
        SIMPLELOG_LOGGER_ERROR(nullptr, "lws_create_context failed");
        return;
    }
    pEndpoint->Status = ELWSStatus::LWS_Idle;
    lwsl_user("Completed\n");
    return;
}

void FWebsocketConnectionManagerLWS::Disconnect(std::shared_ptr<IWebsocketClient>pClient)
{
    if (!pClient) {
        return;
    }
    auto pEndpoint = std::dynamic_pointer_cast<FWebsocketEndpointLWS>(pClient);
    if (pEndpoint->WSI) {
        lws_set_timeout(pEndpoint->WSI, pending_timeout(LWS_TO_KILL_ASYNC), 0);
    }
}

void FWebsocketConnectionManagerLWS::Tick(float delta)
{
    for (auto& pEndpoint : Endpionts) {
        switch (pEndpoint->Status) {
        case ELWSStatus::LWS_Idle: {
            if (!lws_client_connect_via_info(&pEndpoint->LWSConnectInfo)) {
                SIMPLELOG_LOGGER_ERROR(nullptr, "lws_client_connect_via_info failed");
                continue;
            }
            pEndpoint->Status = ELWSStatus::LWS_Connecting;
            break;
        }
        }
        int n = 0;
        n = lws_service(pEndpoint->LWSContext, -1);
    }
}

int FWebsocketConnectionManagerLWS::LWSCallback(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
    const struct msg* pmsg;
    void* retval;
    int n, m, r = 0;

    switch (reason) {

        /* --- protocol lifecycle callbacks --- */

    case LWS_CALLBACK_PROTOCOL_INIT: {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "LWS_CALLBACK_PROTOCOL_INIT Received");
        break;
    }
    case LWS_CALLBACK_PROTOCOL_DESTROY: {
        SIMPLELOG_LOGGER_DEBUG(nullptr, "LWS_CALLBACK_PROTOCOL_DESTROY Received");
        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        auto pEndpoint = reinterpret_cast<FWebsocketEndpointLWS*>(user)->shared_from_this();
        SIMPLELOG_LOGGER_INFO(nullptr, "CLIENT_CONNECTION_ERROR: {}", in ? (char*)in : "");
        pEndpoint->SetError(std::error_code(std::to_underlying(EWSError::WSE_CONNECT_ERROR), ws_category()));
        break;
    }

                                             /* --- client callbacks --- */

    case LWS_CALLBACK_CLIENT_ESTABLISHED: {
        auto pEndpoint = reinterpret_cast<FWebsocketEndpointLWS*>(user)->shared_from_this();
        pEndpoint->Status = ELWSStatus::LWS_Connected;
        pEndpoint->TriggerOnConnectedDelegates(pEndpoint);
        break;
    }
    case LWS_CALLBACK_CLIENT_CLOSED: {
        auto pEndpoint = reinterpret_cast<FWebsocketEndpointLWS*>(user)->shared_from_this();
        pEndpoint->SetError(std::error_code(std::to_underlying(EWSError::WSE_OK), ws_category()));
        break;
    }
    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        auto pEndpoint = reinterpret_cast<FWebsocketEndpointLWS*>(user)->shared_from_this();
        std::shared_ptr<std::vector<uint8_t>> pmsg;
        if (pEndpoint->SendQueue.try_dequeue(pmsg)) {
            enum lws_write_protocol flag = (enum lws_write_protocol)lws_write_ws_flags(LWS_WRITE_BINARY, true, true);
            int m = lws_write(wsi, pmsg->data() + LWS_PRE,
                pmsg->size()- LWS_PRE, flag);
            pEndpoint->SendBufPool.try_enqueue(pmsg);
        }
        if (pEndpoint->SendQueue.size_approx()>0) {
            lws_callback_on_writable(wsi);
        }
        break;
    }
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        auto pEndpoint = reinterpret_cast<FWebsocketEndpointLWS*>(user)->shared_from_this();
        pEndpoint->TriggerOnReceivedDelegates(pEndpoint, (const char*)in, len);
        break;
    }

    default:
        break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

void FWebsocketConnectionManagerLWS::RecycleWebsocketEndpointLWS(std::shared_ptr<FWebsocketEndpointLWS> pEndpoint)
{
    pEndpoint->Clear();
    EndpiontPool.enqueue(pEndpoint);
}
