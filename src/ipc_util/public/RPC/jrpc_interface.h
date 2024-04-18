#pragma once

#include "rpc_processer.h"
#include "message_common.h"

class IPC_EXPORT IGroupJRPC :public IGroupRPC
{
public:
    IGroupJRPC(RPCProcesser* inprocesser) :processer(inprocesser) {}
    bool RespondError(RPCHandle_t handle, double errorCode, const char* errorMsg=nullptr, const char* errorData = nullptr);
protected:
    RPCProcesser* processer;
};