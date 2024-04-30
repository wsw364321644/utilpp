#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include <shared_mutex>

#include "message_internal.h"
#include "delegate_macros.h"
#include "message_session.h"
#pragma warning(push)
#pragma warning(disable:4251)

struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

class IPC_EXPORT MessageServerUV :public IMessageServer
{
    friend class MessageSessionUV;
    friend class UVCallBack;
public:
    MessageServerUV();
    virtual ~MessageServerUV();

public:
    virtual bool OpenServer(EMessageConnectionType, const std::string& url) override;
    virtual void Tick(float delSec) override;
    virtual void Run() override;
    virtual void Stop() override;
    virtual void CloseConnection(IMessageSession*) override;

    virtual EMessageConnectionType GetServerType() const  override {
        return messageConnectionType;
    }
private:

    void CloseServer();


private:

    void UVOnConnection(uv_stream_t* stream, int status);
    void UVOnUDPRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
    void OnSessionClose(MessageSessionUV* session);
    uv_loop_t* loop;
    EMessageConnectionType messageConnectionType;
    union UVHandle_u serverHandle;
    std::vector<std::unique_ptr<MessageSessionUV>> sessions;
    std::shared_mutex sessionMutex;
    uint32_t idCount;
    std::string url;


public:


};
#pragma warning(pop)