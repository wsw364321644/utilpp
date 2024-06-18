///  message_client.h
///  common client

#pragma once
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <map>
#include <thread>
#include "message_internal.h"
#include "delegate_macros.h"
#pragma warning(push)
#pragma warning(disable:4251)

class IPC_EXPORT MessageClientUV : public IMessageClient
{
    friend class UVCallBack;
public:
    MessageClientUV();
    ~MessageClientUV();

public:

    bool Connect(EMessageConnectionType, const std::string& url) override;
    CommonHandle_t Write(const char* data, int len) override;
    void Disconnect()override;
    void Tick(float delSec) override;
    EMessageConnectionType GetConnectionType()const override {
        return messageConnectionType;
    }
    EMessageConnectionState GetConnectionState()const override {
        return state.load();
    }

private:
    //void UVWorker();
    void UVOnConnect(uv_connect_t* req, int status);
    void UVOnClose(uv_handle_t* handle);
    void UVOnShutdown(uv_shutdown_t* req, int status);
    void UVOnRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
    void OnWrite(MessageSendRequestUV*, uv_write_t* req, int status);
    void UVOnDNSResolved(uv_getaddrinfo_t* resolver, int status, struct addrinfo* res);
    void UVOnUDPRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
    void OnUDPSend(MessageSendRequestUV*, uv_udp_send_t* req, int status);
    uv_loop_t* loop;
    EMessageConnectionType messageConnectionType;
    union UVHandle_u clientHandle;
    uv_connect_t connectHandle;

    std::vector<char> readBuf;
    std::vector<char> writeBuf;
    std::map<CommonHandle_t, MessageSendRequestUV> writeRequests;
    std::atomic_uint32_t writeCount;
    std::shared_mutex writeMtx;
    std::thread uvworkThread;
    std::atomic<EMessageConnectionState> state;
    std::atomic_bool running;

};
#pragma warning(pop)