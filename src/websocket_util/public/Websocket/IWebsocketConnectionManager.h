#pragma once
#include <memory>
#include <system_error>
#include <named_class_register.h>
#include "Websocket/IWebsocketClient.h"
class IWebsocketEndpoint;

SIMPLE_NET_EXPORT extern const char LWS_MANAGER_NAME[];

class IWebsocketConnectionManager : public TNamedClassRegister<IWebsocketConnectionManager>
{
public:
    virtual ~IWebsocketConnectionManager() = default;
    virtual std::shared_ptr<IWebsocketClient> CreateClient() = 0;
    virtual void Connect(std::shared_ptr<IWebsocketClient>) = 0;
    virtual void Disconnect(std::shared_ptr<IWebsocketClient>) = 0;
    virtual void Tick(float delta) = 0;
};

SIMPLE_NET_EXPORT IWebsocketConnectionManager* GetWebsocketConnectionManagerSingleton();