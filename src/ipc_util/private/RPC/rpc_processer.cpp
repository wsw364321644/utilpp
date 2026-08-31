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




IRPCProcesser::~IRPCProcesser()
{
}
bool IRPCProcesser::AddGroupRPC(std::string_view groupName) {

    if (RPCAPIInterfaces.contains(groupName)) {
        return true;
    }
    if (RPCInterfaceFactory::GetRPCInfos()) {
        for (auto& [name, info] : *RPCInterfaceFactory::GetRPCInfos()) {
            if (name == groupName) {
                RPCAPIInterfaces.try_emplace(name, RPCInterfaceFactory::Create(name.c_str(), this, &malloc));
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<IGroupRPC> IRPCProcesser::GetInterfaceByMethodName(const char* name)
{
    for (auto& info : *RPCInterfaceFactory::GetRPCInfos()) {
        if (info.second.CheckFunc(name)) {
            return RPCAPIInterfaces.find(info.first)->second;
        }
    }
    return nullptr;
}

FJRPCProcesser::FJRPCProcesser()
{
    rpcParserInterface.reset(new JRPCPaser);
}
FJRPCProcesser::~FJRPCProcesser()
{
}

void FJRPCProcesser::OnRecevRPC(const char* str, uint32_t len, std::error_code& ec)
{
    IRPCPaser::ParseResult parseResult = rpcParserInterface->Parse(str, len);

    auto pResponse = std::get_if<std::shared_ptr<RPCResponse>>(&parseResult);
    if (pResponse) {
        std::shared_ptr<RPCRequest> rpcReq;
        auto response = *pResponse;
        if (!response->HasID()) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        else {
            auto id = response->GetID();
            auto result = requestMap.find(RPCHandle_t(id));
            if (result == requestMap.end()) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return;
            }
            rpcReq = result->second;
            requestMap.erase(result);

            auto rpcInterface = GetInterfaceByMethodName(rpcReq->GetMethod().data());
            if (rpcInterface) {
                if (!RecevRespInGroup(rpcInterface, response, rpcReq)) {
                    ec = std::make_error_code(std::errc::invalid_argument);
                    return;
                }
            }
            else {
                ec = std::make_error_code(std::errc::invalid_argument);
                return;
            }
        }
        return;
    }
    auto pRequest = std::get_if<std::shared_ptr<RPCRequest>>(&parseResult);
    if (pRequest) {
        auto request = *pRequest;
        auto rpcInterface = GetInterfaceByMethodName(request->GetMethod().data());
        if (rpcInterface) {
            if (!RecevReqInGroup(rpcInterface,request)) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return;
            }
        }
        else {
            auto resp=rpcParserInterface->GetMethodNotFoundResponse(request->GetID());
            SendRPCContentDelegate(resp, ec);
            ec = std::make_error_code(std::errc::invalid_argument);
            return;

        }
        return;
    }

    auto ParseError = std::get<ERPCParseError>(parseResult);
    auto resp = rpcParserInterface->GetErrorParseResponse(ParseError);
    SendRPCContentDelegate(resp, ec);
    ec = std::make_error_code(std::errc::invalid_argument);
}

void FJRPCProcesser::OnRecevPacket(MessagePacket_t* p, std::error_code& ec)
{
    OnRecevRPC(p->MessageContent, p->ContentLength,ec);
}


RPCHandle_t FJRPCProcesser::SendRequest(IGroupRPC* group, std::shared_ptr<RPCRequest> request)
{
    std::error_code ec;
    RPCHandle_t handle(counter);
    handle.PIGroupRPC = group;
    request->SetID(handle.ID);

    if (!SendRPCContentDelegate(request, ec)) {
        return NullHandle;
    }
    auto res = requestMap.try_emplace(handle.ID, request);
    if (!res.second) {
        return NullHandle;
    }
    return handle;
}

std::shared_ptr<RPCRequest> FJRPCProcesser::CancelRequest(RPCHandle_t handle)
{
    std::shared_ptr<RPCRequest> preq;
    auto res = requestMap.find(handle);
    if (res == requestMap.end()) {
        return preq;
    }
    preq = res->second;
    requestMap.erase(res);
    return preq;
}

bool FJRPCProcesser::SendEvent(IGroupRPC* group, std::shared_ptr<RPCRequest> request)
{
    std::error_code ec;
    return SendRPCContentDelegate(request, ec);
}

bool FJRPCProcesser::SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response)
{
    std::error_code ec;
    response->SetID(handle.ID);
    return SendRPCContentDelegate(response, ec);
}
