#pragma once
#include "IWebsocketEndpoint.h"

class SIMPLE_NET_EXPORT IWebsocketClient:public IWebsocketEndpoint
{
public:
    virtual bool IsConnected() = 0;
    virtual void SetHost(std::u8string_view) = 0;
    virtual void SetPath(std::u8string_view) = 0;
    DEFINE_EVENT_ONE_PARAM(OnConnected, const std::shared_ptr<IWebsocketClient>&);
    DEFINE_EVENT_TWO_PARAM(OnDisconnected, const std::shared_ptr<IWebsocketClient>&,const std::error_code&);
    DEFINE_EVENT_THREE_PARAM(OnReceived, const std::shared_ptr<IWebsocketClient>&, const char*, size_t);
};