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
    DECLARE_RPC_OVERRIDE_FUNCTION(JRPCCommandAPI);

    DECLARE_RESPONSE_RPC(HeartBeat);
    DECLARE_REQUEST_RPC(HeartBeat);

    DECLARE_RESPONSE_RPC(SetChannel);
    DECLARE_REQUEST_RPC_TWO_PARAM(SetChannel, uint8_t, EMessagePolicy);

};

#pragma warning(pop)
