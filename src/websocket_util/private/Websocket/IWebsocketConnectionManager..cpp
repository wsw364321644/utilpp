#include "Websocket/IWebsocketConnectionManager.h"

IWebsocketConnectionManager* GetWebsocketConnectionManagerSingleton()
{
    auto ptr=IWebsocketConnectionManager::GetNamedClassSingleton(LWS_MANAGER_NAME);
    if (ptr) {
        return ptr.get();
    }
    else {
        return nullptr;
    }
}