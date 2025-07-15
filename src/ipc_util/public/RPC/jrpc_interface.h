#pragma once

#include "rpc_processer.h"
#include "message_common.h"
#include <rapidjson/stringbuffer.h>

class IPC_EXPORT IGroupJRPC :public IGroupRPC
{
public:
    IGroupJRPC(RPCProcesser* inprocesser) :IGroupRPC(inprocesser) {}
    bool RespondError(RPCHandle_t handle, int64_t errorCode, const char* errorMsg=nullptr, const char* errorData = nullptr);
};

#define DEFINE_JRPC_OVERRIDE_FUNCTION(ClassName)                                                             \
bool ClassName::OnRequestRecv(std::shared_ptr<RPCRequest> req)                                               \
{                                                                                                            \
    auto jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);                                              \
    if (!jreq) {                                                                                             \
        return false;                                                                                        \
    }                                                                                                        \
    if (jreq->CheckParams() != ERPCParseError::OK) {                                                         \
        return false;                                                                                        \
    }                                                                                                        \
                                                                                                             \
    auto opt = RPCInfoData<ClassName>::GetMethodInfo(req->GetMethod().data());                               \
    if (!opt.has_value()) {                                                                                  \
        return false;                                                                                        \
    }                                                                                                        \
    opt.value().OnRequestMethod(this, req);                                                                  \
    return true;                                                                                             \
}                                                                                                            \
                                                                                                             \
bool ClassName::OnResponseRecv(std::shared_ptr<RPCResponse> resp, std::shared_ptr<RPCRequest> req)           \
{                                                                                                            \
    auto jresp = std::dynamic_pointer_cast<JsonRPCResponse>(resp);                                           \
    if (!jresp) {                                                                                            \
        return false;                                                                                        \
    }                                                                                                        \
    if (jresp->CheckResult(req->GetMethod().data()) != ERPCParseError::OK) {                                 \
        return false;                                                                                        \
    }                                                                                                        \
    auto opt = RPCInfoData<ClassName>::GetMethodInfo(req->GetMethod().data());                               \
    if (!opt.has_value()) {                                                                                  \
        return false;                                                                                        \
    }                                                                                                        \
    opt.value().OnResponseMethod(this, resp, req);                                                           \
    return true;                                                                                             \
}
