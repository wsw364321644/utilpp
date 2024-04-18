#pragma once

#include <vector>
#include <memory>
#include <uv.h>
#include <atomic>
#include <mutex>
#include <map>
#include "message_common.h"
#pragma warning(push)
#pragma warning(disable:4251)

class MessageServerUV;


class IPC_EXPORT MessageSessionUV : public MessageSessionInterface, public std::enable_shared_from_this<MessageSessionUV>
{
    friend class UVCallBack;
public:

    MessageSessionUV(MessageServerUV* server);
    ~MessageSessionUV();

    bool operator ==(MessageSessionUV& session) {
        return server == session.GetServer() && ID == session.ID;
    }

    virtual EMessageConnectionType GetConnectionType()const override;
    virtual EMessageConnectionState GetConnectionState()const override;
    MessageServerUV* GetServer() {
        return server;
    }

    virtual void Disconnect()override;
    virtual CommonHandle_t Write(const char* data, int len)override;
public:
    uint64_t GetPID();

    void UVOnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    void OnWrite(MessageSendRequestUV*, uv_write_t* req, int status);
    void OnUDPSend(MessageSendRequestUV*, uv_udp_send_t* req, int status);
    void UVOnShutdown(uv_shutdown_t* req, int status);
    void UVOnClose(uv_handle_t* handle);


    UVRemoteHandle_u ClientHandle;
    //std::vector<char> ReadBuf;
    uint32_t ID;



private:
    std::atomic<EMessageConnectionState> state;
    std::atomic_uint32_t writeCount;
    std::map<CommonHandle_t, MessageSendRequestUV> writeRequests;
    MessageServerUV* server;
    uv_shutdown_t shutdownReq;
    std::mutex writeMtx;
    uint32_t workingtime_;

};

#pragma warning(pop)
