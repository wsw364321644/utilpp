#include "RPC/jrpc_command.h"
#include "RPC/message_common.h"
#include "jrpc_parser.h"
#include "rpc_definition.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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
    req->SetMethod(HeartBeatName);
    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddHeartBeatSendDelagate(req->GetID(), inDelegate, errDelegate);
    }
    return handle;
}
bool JRPCCommandAPI::RespondHeartBeat(RPCHandle_t handle)
{

    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->SetError(false);
    return processer->SendResponse(handle, response);
}
void JRPCCommandAPI::OnHeartBeatRequestRecv(std::shared_ptr<RPCRequest> req)
{
    RecvHeartBeatDelegate(RPCHandle_t(req->GetID()));
}
void JRPCCommandAPI::OnHeartBeatResponseRecv(std::shared_ptr<RPCResponse> resp, std::shared_ptr<RPCRequest>req)
{
    auto res = HeartBeatDelegates.find(req->GetID());
    if (res == HeartBeatDelegates.end()) {
        return;
    }
    res->second(RPCHandle_t(req->GetID()));
}


REGISTER_RPC_API_AUTO(JRPCCommandAPI, SetChannel);
DEFINE_REQUEST_RPC(JRPCCommandAPI, SetChannel);
RPCHandle_t JRPCCommandAPI::SetChannel(uint8_t index, EMessagePolicy policy, TSetChannelDelegate inDelegate, TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->SetMethod (SetChannelName);

    rapidjson::Writer<FCharBuffer> writer(req->GetParamsBuf());
    rapidjson::Document doc;
    auto& a = doc.GetAllocator();
    doc.AddMember("index", index, a);
    doc.AddMember("policy", rapidjson::Value(EMessagePolicyInfo::ToString(policy),a), a);
    if (!doc.Accept(writer)) {
        return NullHandle;
    }
    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddSetChannelSendDelagate(req->GetID(), inDelegate, errDelegate);
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