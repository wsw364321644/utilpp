#include "RPC/MessageFactory.h"
#include "message_client.h"
#include "message_server.h"
std::shared_ptr<IMessageServer> NewMessageServer(MessageServerSetting_t Setting) {
    switch (Setting.MessageFoundation) {
    case EMessageFoundation::LIBUV: {
        return std::make_shared<MessageServerUV>() ;
    }
    }
    return nullptr;
}
std::shared_ptr<IMessageClient> NewMessageClient(MessageClientSetting_t Setting) {
    switch (Setting.MessageFoundation) {
    case EMessageFoundation::LIBUV: {
        return std::make_shared<MessageClientUV>();
    }
    }
    return nullptr;
}