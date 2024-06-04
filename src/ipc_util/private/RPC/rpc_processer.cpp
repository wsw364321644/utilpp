#include "RPC/rpc_processer.h"
#include "RPC/message_common.h"
#include "RPC/jrpc_command.h"

#include "jrpc_parser.h"
#include "rpc_definition.h"

#include <delegate_macros.h>
#include <LoggerHelper.h>
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <filesystem>
#include <fstream>


RPCProcesser::RPCProcesser(MessageProcesser* inp) :msgprocesser(inp)
{
    if (!JRPCPaser::bInited) {
        SIMPLELOG_LOGGER_ERROR(nullptr,"JRPCPaser bInited failed");
    }
    if (RPCInterfaceFactory::GetRPCInfos()) {
        for (auto& info : *RPCInterfaceFactory::GetRPCInfos()) {
            RPCAPIInterfaces.emplace(info.first, RPCInterfaceFactory::Create(info.first.c_str(), this, &malloc));
        }
    }
    rpcParserInterface.reset(new JRPCPaser);
    OnPacketRecvHandle= msgprocesser->AddOnPacketRecvDelegate(std::bind(&RPCProcesser::OnRecevPacket, this, std::placeholders::_1));
}
RPCProcesser::~RPCProcesser()
{
    if (OnPacketRecvHandle.IsValid()) {
        msgprocesser->ClearOnPacketRecvDelegate(OnPacketRecvHandle);
    }
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
        if (!response->ID.has_value()) {
            TriggerOnRequestErrorRespondDelegates(response);
        }
        else {
            auto id = response->ID.value();
            {
                std::scoped_lock sl(requestMapMutex);
                auto result = requestMap.find(RPCHandle_t(id));
                if (result == requestMap.end()) {
                    return;
                }
                rpcReq = result->second;
                requestMap.erase(result);
            }
            auto rpcInterface = GetInterfaceByMethodName(rpcReq->Method.c_str());
            if (rpcInterface) {
                rpcInterface->OnResponseRecv(response, rpcReq);
            }
        }
        return;
    }
    auto pRequest = std::get_if<std::shared_ptr<RPCRequest>>(&parseResult);
    if (pRequest) {
        auto request = *pRequest;
        auto rpcInterface = GetInterfaceByMethodName(request->Method.c_str());
        if (rpcInterface) {
            if (!rpcInterface->OnRequestRecv(request)) {
                TriggerOnRPCConsumedErrorDelegates(request);
            }
        }
        else {
            auto buf = rpcParserInterface->GetMethodNotFoundResponse(request->ID)->ToBytes();
            msgprocesser->SendContent(buf.CStr(), buf.Length());
            TriggerOnMethoedNotFoundDelegates(request);
        }
        return;
    }

    auto ParseError = std::get<ERPCParseError>(parseResult);
    auto buf = rpcParserInterface->GetErrorParseResponse(ParseError)->ToBytes();
    msgprocesser->SendContent(buf.CStr(), buf.Length());
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
