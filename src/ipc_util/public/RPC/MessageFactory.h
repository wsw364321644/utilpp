#pragma once
#include <cinttypes>
#include <memory>
#include "message_common.h"
enum class EMessageFoundation:uint8_t
{
    LIBUV,
};

typedef struct MessageServerSetting_t {
    EMessageFoundation MessageFoundation;
}MessageServerSetting_t;

typedef struct MessageClientSetting_t {
    EMessageFoundation MessageFoundation;
}MessageClientSetting_t;


IPC_EXPORT std::shared_ptr<IMessageServer> NewMessageServer(MessageServerSetting_t);
IPC_EXPORT std::shared_ptr<IMessageClient> NewMessageClient(MessageClientSetting_t);