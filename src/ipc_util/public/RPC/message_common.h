#pragma once

#include <regex>
#include <functional>
#include <variant>
#include <memory>
#include <vector>
#include "ipc_util_export.h"
#include "delegate_macros.h"

enum class EMessageConnectionType {
    EMCT_TCP,
    EMCT_IPC,
    EMCT_UDP
};
enum class EMessageError {
    OK,
    ParseError,
    InternalError
};


enum class EMessageConnectionState :uint8_t {
    Idle,
    InitConnect,
    Connecting,
    Connected,
    WaitClose,
    Closing
};


class IPC_EXPORT IMessageSession {
public:
    virtual EMessageConnectionType GetConnectionType()const = 0;
    virtual void Disconnect() = 0;
    virtual CommonHandle_t Write(const char* data, int len) = 0;
    virtual EMessageConnectionState GetConnectionState()const = 0;
    DEFINE_EVENT_ONE_PARAM(OnDisconnect, IMessageSession*);
    DEFINE_EVENT_THREE_PARAM(OnRead, IMessageSession*, char*, intptr_t);
    DEFINE_EVENT_THREE_PARAM(OnWrite, IMessageSession*, CommonHandle_t,int);
};

class IPC_EXPORT IMessageClient:public IMessageSession {
public:
    virtual bool Connect(EMessageConnectionType, const std::string& url)=0;
    DEFINE_EVENT_ONE_PARAM(OnConnect, IMessageClient*);
};

class IPC_EXPORT IMessageServer  {
public:
    virtual bool OpenServer(EMessageConnectionType, const std::string& url) = 0;
    virtual void Tick(float delSec) = 0;
    virtual void Run() = 0;
    virtual void Stop() = 0;
    virtual void CloseConnection(IMessageSession*) = 0;
    DEFINE_EVENT_ONE_PARAM(OnConnect, IMessageSession*);
};
