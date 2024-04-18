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




//std::unique_ptr<IGroupRPC> DefaultJRPCCommandAPI= JRPCCommandAPI::Create(nullptr);
//std::unordered_map<std::string, RPCMethodInfo<JRPCCommandAPI>> JRPCCommandAPI::rpcInfoData;



std::unordered_map<std::string, RPCMethodInfo<JRPCCommandAPI>> RPCInfoData<JRPCCommandAPI>::MethodInfos;
JRPCCommandAPI::JRPCCommandAPI(RPCProcesser* inprocesser) :IGroupJRPC(inprocesser)
{
}
JRPCCommandAPI::~JRPCCommandAPI()
{
}

std::unique_ptr<IGroupRPC> JRPCCommandAPI::Create(RPCProcesser* inprocesser, RPCInterfaceInfo::fnnew fn)
{
    if (fn) {
        JRPCCommandAPI* ptr = (JRPCCommandAPI*)fn(sizeof(JRPCCommandAPI));
        new(ptr)JRPCCommandAPI(inprocesser);

        return  std::unique_ptr<JRPCCommandAPI>(ptr);
    }
    return std::make_unique<JRPCCommandAPI>(inprocesser);
}

const char* JRPCCommandAPI::GetGroupName()
{
    return "command";
}
void JRPCCommandAPI::StaticInit(bool(*func)(const char* name, RPCInterfaceInfo info))
{
    func(JRPCCommandAPI::GetGroupName(), { .CreateFunc = &JRPCCommandAPI::Create ,.CheckFunc = &RPCInfoData<JRPCCommandAPI>::CheckMethod });
}
bool JRPCCommandAPI::OnRequestRecv(std::shared_ptr<RPCRequest> req)
{
    auto opt = RPCInfoData<JRPCCommandAPI>::GetMethodInfo(req->Method.c_str());
    if (!opt.has_value()) {
        return false;
    }
    opt.value().OnRequestMethod(this, req);
    return true;
}

bool JRPCCommandAPI::OnResponseRecv(std::shared_ptr<RPCResponse> res, std::shared_ptr<RPCRequest> req)
{
    auto opt = RPCInfoData<JRPCCommandAPI>::GetMethodInfo(req->Method.c_str());
    if (!opt.has_value()) {
        return false;
    }
    opt.value().OnResponseMethod(this, res, req);
    return true;
}

//static const char HeartBeatName[] = "HeartBeat";
//static RPCInfoRegister<JRPCCommandAPI> HeartBeatRegister(RPCMethodInfo<JRPCCommandAPI>{.Name = HeartBeatName,
//    .OnRequestMethod = &JRPCCommandAPI::OnHeartBeatRequestRecv,
//    .OnResponseMethod = &JRPCCommandAPI::OnHeartBeatResponseRecv
//});
REGISTER_RPC_API_AUTO(HeartBeat, JRPCCommandAPI);
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


REGISTER_RPC_API_AUTO(SetChannel, JRPCCommandAPI);
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