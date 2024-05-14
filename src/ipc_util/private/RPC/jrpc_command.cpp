#include "RPC/jrpc_command.h"
#include "RPC/message_common.h"
#include "jrpc_parser.h"
#include "rpc_definition.h"
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <map>
#include <string>
#include <chrono>
#include <list>
#include <sstream>
#include <cassert>

DEFINE_RPC_OVERRIDE_FUNCTION(JRPCCommandAPI, "command");
DEFINE_JRPC_OVERRIDE_FUNCTION(JRPCCommandAPI);

REGISTER_RPC_API_AUTO(JRPCCommandAPI, HeartBeat);
DEFINE_REQUEST_RPC(JRPCCommandAPI, HeartBeat);
RPCHandle_t JRPCCommandAPI::HeartBeat(THeartBeatDelegate inDelegate,TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = HeartBeatName;
    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddHeartBeatSendDelagate(req->ID.value(), inDelegate, errDelegate);
    }
    return handle;
}
bool JRPCCommandAPI::RespondHeartBeat(RPCHandle_t handle)
{

    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->OptError = false;
    return processer->SendResponse(handle, response);
}
void JRPCCommandAPI::OnHeartBeatRequestRecv(std::shared_ptr<RPCRequest> req)
{
    recvHeartBeatDelegate(RPCHandle_t(req->ID.value()));
}
void JRPCCommandAPI::OnHeartBeatResponseRecv(std::shared_ptr<RPCResponse> resp, std::shared_ptr<RPCRequest>req)
{
    auto res = HeartBeatDelegates.find(req->ID.value());
    if (res == HeartBeatDelegates.end()) {
        return;
    }
    res->second(RPCHandle_t(req->ID.value()));
}


REGISTER_RPC_API_AUTO(JRPCCommandAPI, SetChannel);
DEFINE_REQUEST_RPC(JRPCCommandAPI, SetChannel);
RPCHandle_t JRPCCommandAPI::SetChannel(uint8_t index, EMessagePolicy policy, TSetChannelDelegate inDelegate, TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->Method = SetChannelName;

    nlohmann::json doc(nlohmann::json::value_t::object);
    doc["index"] = index;
    doc["policy"] = EMessagePolicyInfo::ToString(policy);
    req->Params = doc.dump();

    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddSetChannelSendDelagate(req->ID.value(), inDelegate, errDelegate);
    }
    return handle;
}

bool JRPCCommandAPI::RespondSetChannel(RPCHandle_t handle)
{
    return false;
}

void JRPCCommandAPI::OnSetChannelRequestRecv(std::shared_ptr<RPCRequest>)
{
}

void JRPCCommandAPI::OnSetChannelResponseRecv(std::shared_ptr<RPCResponse>, std::shared_ptr<RPCRequest>)
{
}