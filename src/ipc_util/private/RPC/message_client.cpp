#include "RPC/message_client.h"
#include "message_internal.h"
#include "logger.h"
#include <uv.h>
#include <uri.h>

#define TIMERREPEAT 1000 *60

MessageClientUV::MessageClientUV() :messageConnectionType(EMessageConnectionType::EMCT_IPC), state(EMessageConnectionState::Idle), running(true)
, writeCount(0)
{
    loop = new uv_loop_t;
    connectHandle.data = this;
    //uvworkThread = std::thread(&MessageClientUV::UVWorker, this);
}
MessageClientUV::~MessageClientUV()
{
    running = false;
    if (uvworkThread.joinable()) {
        uvworkThread.join();
    }
    delete loop;
}
bool MessageClientUV::Connect(EMessageConnectionType type, const std::string& url)
{
    EMessageConnectionState expected = EMessageConnectionState::Idle;
    if (!state.compare_exchange_strong(expected, EMessageConnectionState::InitConnect)) {
        return false;
    }
    uv_loop_init(loop);

    messageConnectionType = type;
    std::string schemestr, authoritystr, portstr;
    ParsedURL_t parsedURL;
    parsedURL.outScheme = &schemestr;
    parsedURL.outAuthority = &authoritystr;
    parsedURL.outPort = &portstr;
    ParseUrl(url, &parsedURL);

    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& pipeHandle = clientHandle.PipeHandle;
        pipeHandle.data = this;
        if (uv_pipe_init(loop, &pipeHandle, 0) != 0) {
            return false;
        }
        uv_pipe_connect(&connectHandle, &pipeHandle, url.c_str(), UVCallBack::template UVOnConnect<MessageClientUV>);
        break;
    }

    case EMessageConnectionType::EMCT_TCP:
    case EMessageConnectionType::EMCT_UDP: {
        auto port = std::stoi(portstr);
        if (port <= 0) {
            return false;
        }

        struct addrinfo hints;
        hints.ai_family = PF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = 0;
        uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t;
        resolver->data = this;
        uv_getaddrinfo(loop, resolver, UVCallBack::template UVOnDNSResolved<MessageClientUV>, authoritystr.c_str(), portstr.c_str(), &hints);
        break;
    }
    }

    expected = EMessageConnectionState::InitConnect;
    if (!state.compare_exchange_strong(expected, EMessageConnectionState::Connecting)) {
        return false;
    }

    return true;
}
CommonHandle_t MessageClientUV::Write(const char* data, int len)
{
    CommonHandle_t res(NullHandle);
    if (state.load() != EMessageConnectionState::Connected) {
        return res;
    }
    writeMtx.lock();
    auto pair = writeRequests.emplace(writeCount, messageConnectionType);
    writeMtx.unlock();
    if (!pair.second) {
        return res;
    }
    res = pair.first->first;
    auto& req = pair.first->second;
    req.Handle = res;

    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        req.Write((uv_stream_t*)&clientHandle.PipeHandle, data, len, std::bind(&MessageClientUV::OnWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        req.Write((uv_stream_t*)&clientHandle.TCPHandle, data, len, std::bind(&MessageClientUV::OnWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        req.UDPSend(&clientHandle.UDPHandle, data, len, std::bind(&MessageClientUV::OnUDPSend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        break;
    }
    }
    return res;
}

void MessageClientUV::Disconnect()
{
    EMessageConnectionState expected = EMessageConnectionState::Connected;
    if (!state.compare_exchange_strong(expected, EMessageConnectionState::Closing)) {
        LOG_ERROR("Disconnect state error {}", (uint8_t)expected);
    }

    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& handle = clientHandle.PipeHandle;
        //handle.data = shutdownDelegate.get();
        uv_shutdown_t* req = new uv_shutdown_t;
        req->data = this;
        uv_shutdown(req, (uv_stream_t*)&handle, UVCallBack::template UVOnShutdown< MessageClientUV>);
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& handle = clientHandle.TCPHandle;
        //handle.data = shutdownDelegate.get();
        uv_shutdown_t* req = new uv_shutdown_t;
        req->data = this;
        uv_shutdown(req, (uv_stream_t*)&handle, UVCallBack::template UVOnShutdown< MessageClientUV>);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        auto& handle = clientHandle.UDPHandle;
        //handle.data = shutdownDelegate.get();
        uv_shutdown_t* req = new uv_shutdown_t;
        req->data = this;
        uv_shutdown(req, (uv_stream_t*)&handle, UVCallBack::template UVOnShutdown< MessageClientUV>);
        break;
    }
    }
    return;
}

void MessageClientUV::Tick(float delSec)
{
    auto curstate = state.load();
    if (curstate != EMessageConnectionState::Idle && curstate != EMessageConnectionState::InitConnect) {
        uv_run(loop, uv_run_mode::UV_RUN_NOWAIT);
    }
}

//void MessageClientUV::UVWorker()
//{
//    while (running) {
//        auto curstate = state.load();
//        if (curstate != EMessageConnectionState::Idle && curstate != EMessageConnectionState::InitConnect) {
//            uv_run(loop, uv_run_mode::UV_RUN_NOWAIT);
//        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(33));
//    }
//}

void MessageClientUV::UVOnConnect(uv_connect_t* req, int status)
{
    if (status < 0) {
        LOG_INFO("MessageClientUV failed to connect status:{}", status);
        TriggerOnDisconnectDelegates(this);
        EMessageConnectionState expected = EMessageConnectionState::Connecting;
        if (!state.compare_exchange_strong(expected, EMessageConnectionState::Idle)) {
            LOG_ERROR("OnConnection state error {}", (uint8_t)expected);
            return;
        }
        return;
    }
    EMessageConnectionState expected = EMessageConnectionState::Connecting;
    if (!state.compare_exchange_strong(expected, EMessageConnectionState::Connected)) {
        LOG_ERROR("OnConnection state error {}", (uint8_t)expected);
        return;
    }
    uv_stream_t* stream = req->handle;
    stream->data = this;
    uv_read_start(stream, UVCallBack::UVOnAlloc, UVCallBack::template UVOnRead<MessageClientUV>);

    TriggerOnConnectDelegates(this);
}
void MessageClientUV::UVOnClose(uv_handle_t* handle)
{
    EMessageConnectionState expected = EMessageConnectionState::Closing;
    if (!state.compare_exchange_strong(expected, EMessageConnectionState::Idle)) {
        LOG_ERROR("OnClose state error {}", (uint8_t)expected);
    }
    TriggerOnDisconnectDelegates(this);
}
void MessageClientUV::UVOnShutdown(uv_shutdown_t* req, int status)
{
    auto curstate = state.load();
    if (curstate != EMessageConnectionState::Closing) {
        LOG_ERROR("OnShutdown state error {}", (uint8_t)curstate);
    }
    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& handle = clientHandle.PipeHandle;
        //handle.data = closeDelegate.get();
        uv_close((uv_handle_t*)&handle, UVCallBack::template UVOnClose<MessageClientUV>);
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& handle = clientHandle.TCPHandle;
        //handle.data = closeDelegate.get();
        uv_close((uv_handle_t*)&handle, UVCallBack::template UVOnClose<MessageClientUV>);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        auto& handle = clientHandle.UDPHandle;
        //handle.data = closeDelegate.get();
        uv_close((uv_handle_t*)&handle, UVCallBack::template UVOnClose<MessageClientUV>);
        break;
    }
    }
    delete req;
}
void MessageClientUV::UVOnRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    if (nread < 0) {
        if (nread != UV_EOF) {
            LOG_WARNING("{}, Read from remote error: {}", Log_Server, nread);
        }
        Disconnect();
        return;
    }
    if (nread > 0) {

        TriggerOnReadDelegates(this, buf->base, nread);
    }
    UVCallBack::UVOnFree(buf);
}
void MessageClientUV::OnWrite(MessageSendRequestUV* msreq, uv_write_t* req, int status)
{
    TriggerOnWriteDelegates(this, msreq->Handle,status);
    writeMtx.lock();
    auto itr = writeRequests.find(msreq->Handle);
    //auto itr=std::find_if(writeRequests.begin(), writeRequests.end(), [&](const UVWriteRequest& inreq)->bool {
    //    return &inreq.sendReq.SendReq == req;
    //    });
    if (itr == writeRequests.end()) {
        LOG_ERROR("OnWrite req error");
        return;
    }
    writeRequests.erase(itr);
    writeMtx.unlock();
}
void MessageClientUV::UVOnDNSResolved(uv_getaddrinfo_t* resolver, int status, addrinfo* res)
{
    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_TCP: {
        uv_tcp_t* handle = &clientHandle.TCPHandle;
        uv_tcp_init(loop, handle);
        handle->data = this;
        uv_tcp_keepalive(handle, 1, 60);

        uv_tcp_connect(&connectHandle, handle, res->ai_addr, UVCallBack::template UVOnConnect<MessageClientUV>);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {

        uv_udp_t* udpHandle = &clientHandle.UDPHandle;
        if (uv_udp_init(loop, udpHandle) != 0) {
            return;
        }
        udpHandle->data = this;
        uv_udp_set_broadcast(udpHandle, 0);
        //uv_udp_bind(udpHandle, uv_ip4_addr("0.0.0.0", 0), 0);
        if (uv_udp_connect(udpHandle, res->ai_addr) < 0) {
            Disconnect();
            return;
        }

        uv_udp_recv_start(udpHandle, UVCallBack::UVOnAlloc, UVCallBack::template UVOnUDPRecv<MessageClientUV>);
        EMessageConnectionState expected = EMessageConnectionState::Connecting;
        if (!state.compare_exchange_strong(expected, EMessageConnectionState::Connected)) {
            LOG_ERROR("udp connnect state error {}", (uint8_t)expected);
        }
        TriggerOnConnectDelegates(this);
        break;
    }
    }
    uv_freeaddrinfo(res);
    delete resolver;
}
void MessageClientUV::UVOnUDPRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const sockaddr* addr, unsigned flags)
{
    if (nread < 0) {
        // log and quit
        if (nread != UV_EOF) {
            LOG_WARNING("{}, Read from remote error: {}", Log_Server, nread);
        }
        Disconnect();
        return;
    }
    if (nread > 0) {
        TriggerOnReadDelegates(this, buf->base, nread);
    }
    UVCallBack::UVOnFree(buf);
}

void MessageClientUV::OnUDPSend(MessageSendRequestUV* msreq, uv_udp_send_t* req, int status)
{
    TriggerOnWriteDelegates(this, msreq->Handle,status);
    writeMtx.lock();
    auto itr = writeRequests.find(msreq->Handle);
    //auto itr = std::find_if(writeRequests.begin(), writeRequests.end(), [&](const UVWriteRequest& inreq)->bool {
    //    return &inreq.sendReq.UDPSendReq == req;
    //    });
    if (itr == writeRequests.end()) {
        LOG_ERROR("OnUDPSend req error");
        return;
    }
    writeRequests.erase(itr);
    writeMtx.unlock();
}