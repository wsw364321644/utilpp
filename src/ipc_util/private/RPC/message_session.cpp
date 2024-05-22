#include "RPC/message_session.h"
#include "RPC/message_server.h"
#include "message_internal.h"
#include <logger.h>
#include <module_util.h>


MessageSessionUV::MessageSessionUV(MessageServerUV* inserver)
    : server(inserver), state(EMessageConnectionState::Connected), workingtime_(0)
{
    switch (GetServer()->GetServerType()) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& handle = ClientHandle.PipeHandle;
        handle.data = this;
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& handle = ClientHandle.TCPHandle;
        handle.data = this;
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {

        break;
    }
    }
    shutdownReq.data = this;
}

MessageSessionUV::~MessageSessionUV()
{
}

EMessageConnectionType MessageSessionUV::GetConnectionType() const
{
    return  server->GetServerType();
}

EMessageConnectionState MessageSessionUV::GetConnectionState() const
{
    return state.load();
}

void MessageSessionUV::Disconnect()
{
    EMessageConnectionState expect = EMessageConnectionState::Connected;
    if (!state.compare_exchange_strong(expect, EMessageConnectionState::Closing)) {
        return;
    }
    switch (GetServer()->GetServerType()) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& handle = ClientHandle.PipeHandle;
        uv_read_stop((uv_stream_t*)&handle);
        uv_shutdown(&shutdownReq, (uv_stream_t*)&handle, UVCallBack::template UVOnShutdown<MessageSessionUV>);
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& handle = ClientHandle.TCPHandle;
        uv_read_stop((uv_stream_t*)&handle);
        uv_shutdown(&shutdownReq, (uv_stream_t*)&handle, UVCallBack::template UVOnShutdown<MessageSessionUV>);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        //auto& udpHandle = session->ClientHandle.UDPHandle;
        //uv_close((uv_handle_t*)&udpHandle, MessageServer::UVOnConnectionClosed);
        GetServer()->OnSessionClose(this);
        break;
    }
    }
}

CommonHandle_t MessageSessionUV::Write(const char* data, int len)
{
    auto res = CommonHandle_t(NullHandle);
    auto messageConnectionType = GetServer()->GetServerType();
    writeMtx.lock();
    auto pair = writeRequests.emplace(writeCount, messageConnectionType);
    writeMtx.unlock();
    if (!pair.second) {
        return res;
    }
    res = pair.first->first;
    auto& req = pair.first->second;
    req.Handle = res;

    if (state.load() != EMessageConnectionState::Connected) {
        return res;
    }
    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        req.Write((uv_stream_t*)&ClientHandle.PipeHandle, data, len, std::bind(&MessageSessionUV::OnWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        req.Write((uv_stream_t*)&ClientHandle.TCPHandle, data, len, std::bind(&MessageSessionUV::OnWrite, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        req.UDPSend(&(GetServer()->serverHandle.UDPHandle), data, len, ClientHandle.RemoteAddr.GetAdddr(), std::bind(&MessageSessionUV::OnUDPSend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        break;
    }
    }
    return res;
}

uint64_t MessageSessionUV::GetPID() const
{
    auto messageConnectionType = server->GetServerType();
    if (messageConnectionType != EMessageConnectionType::EMCT_IPC) {
        return NULL;
    }
    uint64_t out;
    GetProcessIdFromPipe(ClientHandle.PipeHandle.handle, &out);
    return out;
}


void MessageSessionUV::UVOnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{

    if (nread <= 0) {
        // log and quit
        if (nread != UV_EOF) {
            LOG_WARNING("{}, Read from remote error: {}", Log_Server, nread);
        }
        Disconnect();
        return;
    }
    //ReadBuf.insert(ReadBuf.end(), buf->base, buf->base + nread);
    TriggerOnReadDelegates(this, buf->base, nread);
    UVCallBack::UVOnFree(buf);
}



void MessageSessionUV::OnWrite(MessageSendRequestUV* mreq, uv_write_t* req, int status)
{
    TriggerOnWriteDelegates(this, mreq->Handle,status);
    writeMtx.lock();
    auto itr = writeRequests.find(mreq->Handle);
    if (itr == writeRequests.end()) {
        LOG_ERROR("OnWrite req error");
    }
    else {
        writeRequests.erase(itr);
    }
    writeMtx.unlock();
}

void MessageSessionUV::OnUDPSend(MessageSendRequestUV* mreq, uv_udp_send_t* req, int status)
{
    TriggerOnWriteDelegates(this, mreq->Handle, status);
    writeMtx.lock();
    auto itr = writeRequests.find(mreq->Handle);
    if (itr == writeRequests.end()) {
        LOG_ERROR("OnUDPSend req error");
    }
    else {
        writeRequests.erase(itr);
    }
    writeMtx.unlock();
}

void MessageSessionUV::UVOnShutdown(uv_shutdown_t* req, int status)
{
    switch (GetServer()->GetServerType()) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& handle = ClientHandle.PipeHandle;
        uv_close((uv_handle_t*)&handle, UVCallBack::template UVOnClose<MessageSessionUV>);
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& handle = ClientHandle.TCPHandle;
        uv_close((uv_handle_t*)&handle, UVCallBack::template UVOnClose<MessageSessionUV>);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        //auto& udpHandle = session->ClientHandle.UDPHandle;
        //uv_close((uv_handle_t*)&udpHandle, MessageServer::UVOnConnectionClosed);
        break;
    }
    }
}

void MessageSessionUV::UVOnClose(uv_handle_t* handle)
{
    EMessageConnectionState expect = EMessageConnectionState::Closing;
    if (!state.compare_exchange_strong(expect, EMessageConnectionState::Idle)) {
        return;
    }
    GetServer()->OnSessionClose(this);

}