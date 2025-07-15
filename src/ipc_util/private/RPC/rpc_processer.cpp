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

static thread_local FCharBuffer SendBuf;
static FCharBuffer Recevbuf;

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
        if (!response->HasID()) {
            TriggerOnRequestErrorRespondDelegates(response);
        }
        else {
            auto id = response->GetID();
            {
                std::scoped_lock sl(requestMapMutex);
                auto result = requestMap.find(RPCHandle_t(id));
                if (result == requestMap.end()) {
                    return;
                }
                rpcReq = result->second;
                requestMap.erase(result);
            }
            auto rpcInterface = GetInterfaceByMethodName(rpcReq->GetMethod().data());
            if (rpcInterface) {
                rpcInterface->OnResponseRecv(response, rpcReq);
            }
        }
        return;
    }
    auto pRequest = std::get_if<std::shared_ptr<RPCRequest>>(&parseResult);
    if (pRequest) {
        auto request = *pRequest;
        auto rpcInterface = GetInterfaceByMethodName(request->GetMethod().data());
        if (rpcInterface) {
            if (!rpcInterface->OnRequestRecv(request)) {
                TriggerOnRPCConsumedErrorDelegates(request);
            }
        }
        else {
            Recevbuf.Clear();
            rpcParserInterface->GetMethodNotFoundResponse(request->GetID())->ToBytes(Recevbuf);
            msgprocesser->SendContent(Recevbuf.CStr(), Recevbuf.Length());
            TriggerOnMethoedNotFoundDelegates(request);
        }
        return;
    }

    auto ParseError = std::get<ERPCParseError>(parseResult);
    Recevbuf.Clear();
    rpcParserInterface->GetErrorParseResponse(ParseError)->ToBytes(Recevbuf);
    msgprocesser->SendContent(Recevbuf.CStr(), Recevbuf.Length());
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
        request->SetID( handle.ID);
        SendBuf.Clear();
        request->ToBytes(SendBuf);
        if (msgprocesser->SendContent(SendBuf.CStr(), (uint32_t)SendBuf.Length())) {
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
    SendBuf.Clear();
    request->ToBytes(SendBuf);
    return msgprocesser->SendContent(SendBuf.CStr(), (uint32_t)SendBuf.Length());
}
bool RPCProcesser::SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response)
{
    response->SetID(handle.ID);
    SendBuf.Clear();
    response->ToBytes(SendBuf);
    return msgprocesser->SendContent(SendBuf.CStr(), (uint32_t)SendBuf.Length());
}
