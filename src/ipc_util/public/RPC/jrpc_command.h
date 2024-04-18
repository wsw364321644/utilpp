#pragma once

#include "rpc_command_interface.h"
#include "jrpc_interface.h"
#include "rpc_processer.h"
#include "message_processer.h"
#pragma warning(push)
#pragma warning(disable:4251)


class IPC_EXPORT JRPCCommandAPI :public IGroupJRPC, public IRPCCommandAPI
{
public:
    JRPCCommandAPI(RPCProcesser*);
    virtual ~JRPCCommandAPI();

    static std::unique_ptr<IGroupRPC> Create(RPCProcesser* inprocesser, RPCInterfaceInfo::fnnew);
    static  void StaticInit(bool(*func)(const char* name, RPCInterfaceInfo info));
    static  const char* GetGroupName();
    virtual const char* GetName() {
        return GetGroupName();
    }
    virtual bool OnRequestRecv(std::shared_ptr<RPCRequest>) override;
    virtual bool OnResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>) override;

    DECLARE_RESPONSE_RPC(HeartBeat);
    DECLARE_REQUEST_RPC(HeartBeat);

    DECLARE_RESPONSE_RPC(SetChannel);
    DECLARE_REQUEST_RPC_TWO_PARAM(SetChannel, uint8_t, EMessagePolicy);


private:

    //TRecvHeartBeatDelegate recvHeartBeatDelegate;
    //std::unordered_map<uint32_t, THeartBeatDelegate> HeartBeatDelegates;
    //TRecvSetChannelDelegate recvSetChannelDelegate;
    //std::unordered_map<uint32_t, TSetChannelDelegate> SetChannelDelegates;

    //RPCProcesser* processer;
};
#pragma warning(pop)
