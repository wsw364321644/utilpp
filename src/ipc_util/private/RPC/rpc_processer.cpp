#include "RPC/rpc_processer.h"
#include "RPC/message_common.h"
#include "RPC/jrpc_command.h"

#include "jrpc_parser.h"
#include "rpc_definition.h"

#include <delegate_macros.h>
#include <logger.h>
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <filesystem>
#include <fstream>


RPCProcesser::RPCProcesser(MessageProcesser* inp) :msgprocesser(inp)
{
    if (!JRPCPaser::bInited) {
        LOG_ERROR("JRPCPaser bInited failed");
    }
    if (RPCInterfaceFactory::GetRPCInfos()) {
        for (auto& info : *RPCInterfaceFactory::GetRPCInfos()) {
            RPCAPIInterfaces.emplace(info.first, RPCInterfaceFactory::Create(info.first.c_str(), this, &malloc));
        }
    }
    rpcParserInterface.reset(new JRPCPaser);
    inp->AddOnPacketRecvDelegate(std::bind(&RPCProcesser::OnRecevPacket, this, std::placeholders::_1));
}
RPCProcesser::~RPCProcesser()
{
}

std::shared_ptr<IGroupRPC> RPCProcesser::GetInterfaceByMethodName(const char* name)
{
    for (auto& info : *RPCInterfaceFactory::GetRPCInfos()) {
        if (info.second.CheckFunc(name)) {
            return RPCAPIInterfaces.find(info.first)->second;
        }
    }
    return nullptr;
}

void RPCProcesser::OnRecevRPC(const char* str, uint32_t len)
{
    IRPCPaser::ParseResult parseResult = rpcParserInterface->Parse(str, len);
    auto pResponse = std::get_if<std::shared_ptr<RPCResponse>>(&parseResult);

    if (pResponse) {
        std::shared_ptr<RPCRequest> rpcReq;
        auto response = *pResponse;
        auto id = response->ID;
        {
            std::scoped_lock sl(requestMapMutex);
            auto result = requestMap.find(RPCHandle_t(id));
            if (result == requestMap.end()) {
                return;
            }
            rpcReq = result->second;
            requestMap.erase(result);
        }
        GetInterfaceByMethodName(rpcReq->Method.c_str())->OnResponseRecv(response, rpcReq);
    }
    auto pRequest = std::get_if<std::shared_ptr<RPCRequest>>(&parseResult);
    if (pRequest) {
        auto request = *pRequest;
        if (GetInterfaceByMethodName(request->Method.c_str())) {
            if (!GetInterfaceByMethodName(request->Method.c_str())->OnRequestRecv(request)) {
                TriggerOnRPCConsumedErrorDelegates(request);
            }
        }
        else {
            TriggerOnMethoedNotFoundDelegates(request);
        }
    }
}

void RPCProcesser::OnRecevPacket(MessagePacket_t* p)
{
    OnRecevRPC(p->MessageContent, p->ContentLength);
}


RPCHandle_t RPCProcesser::SendRequest(std::shared_ptr<RPCRequest> request)
{
    std::unique_lock lock(requestMapMutex);
    auto res = requestMap.emplace(RPCHandle_t(counter), request);
    lock.unlock();
    if (res.second) {
        auto handle = res.first->first;
        request->ID = handle.ID;
        const auto& str = request->ToBytes();
        if (msgprocesser->SendContent(str.CStr(), (uint32_t)str.Length())) {
            return handle;
        }
        else {
            lock.lock();
            requestMap.erase(res.first);
            lock.unlock();
            return NullHandle;
        }

    }
    else {
        return NullHandle;
    }

}
std::shared_ptr<RPCRequest> RPCProcesser::CancelRequest(RPCHandle_t handle)
{
    std::shared_ptr<RPCRequest> preq;
    {
        std::scoped_lock lock(requestMapMutex);
        auto res = requestMap.find(handle);
        if (res == requestMap.end()) {
            return preq;
        }
        preq = res->second;
        requestMap.erase(res);
    }
    return preq;

}
bool RPCProcesser::SendEvent(std::shared_ptr<RPCRequest> request)
{
    const auto& str = request->ToBytes();
    return msgprocesser->SendContent(str.CStr(), (uint32_t)str.Length());
}
bool RPCProcesser::SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response)
{
    response->ID = handle.ID;
    const auto str = response->ToBytes();
    return msgprocesser->SendContent(str.CStr(), (uint32_t)str.Length());
}
