#pragma once
#include "RpcHookHelperInterface.h"
#include <RPC/jrpc_interface.h>
#include <RPC/rpc_processer.h>


class HOOK_HELPER_EXPORT JRPCHookHelperAPI :public IGroupJRPC, public IRPCHookHelperAPI
{
public:

    DECLARE_RPC_OVERRIDE_FUNCTION(JRPCHookHelperAPI);

    DECLARE_RESPONSE_RPC(ConnectToHost);
    DECLARE_REQUEST_RPC_TWO_PARAM(ConnectToHost, uint64_t, const char*);
    DECLARE_RESPONSE_RPC(AddWindow);
    DECLARE_REQUEST_RPC_TWO_PARAM(AddWindow, uint64_t, const char*);
    DECLARE_RESPONSE_RPC(RemoveWindow);
    DECLARE_REQUEST_RPC_ONE_PARAM(RemoveWindow, uint64_t);

private:

};