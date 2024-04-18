#pragma once

#include <functional>
#include "rpc_interface.h"
#include "message_processer.h"


class IPC_EXPORT IRPCCommandAPI
{
public:
    //typedef std::function<void(RPCHandle_t)> THeartBeatDelegate;
    //virtual RPCHandle_t HeartBeat(THeartBeatDelegate) = 0;
    //typedef std::function<void(RPCHandle_t)> TRecvHeartBeatDelegate;
    //virtual void RegisterRecvHeartBeat(TRecvHeartBeatDelegate) = 0;
    //virtual bool RespondHeartBeat(RPCHandle_t id) = 0;


    //typedef std::function<void(RPCHandle_t, bool)> TSetChannelDelegate;
    //virtual RPCHandle_t SetChannel(uint8_t index, EMessagePolicy policy, TSetChannelDelegate) = 0;
    //typedef std::function<void(RPCHandle_t, uint8_t, EMessagePolicy)> TRecvSetChannelDelegate;
    //virtual void RegisterRecvSetChannel(TRecvSetChannelDelegate) = 0;
    //virtual bool RespondSetChannel(RPCHandle_t id, bool res) = 0;

};