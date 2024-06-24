#include "RPC/JrpcHookHelper.h"

#include <jrpc_parser.h>
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <map>
#include <string>
#include <chrono>
#include <list>
#include <RPC/message_common.h>

DEFINE_RPC_OVERRIDE_FUNCTION(JRPCHookHelperAPI, "HookHelper");
DEFINE_JRPC_OVERRIDE_FUNCTION(JRPCHookHelperAPI);

REGISTER_RPC_API_AUTO(JRPCHookHelperAPI, ConnectToHost);
DEFINE_REQUEST_RPC(JRPCHookHelperAPI, ConnectToHost);
RPCHandle_t JRPCHookHelperAPI::ConnectToHost(uint64_t processId, const char* commandline, TConnectToHostDelegate inDelegate, TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->SetMethod(ConnectToHostName);

    nlohmann::json obj = nlohmann::json::object();
    obj["processId"] = processId;
    obj["commandline"] = commandline;
    req->SetParams(obj.dump());

    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddConnectToHostSendDelagate(req->GetID(), inDelegate, errDelegate);
    }
    return handle;
}

bool JRPCHookHelperAPI::RespondConnectToHost(RPCHandle_t handle)
{
    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->OptError = false;

    nlohmann::json doc = nlohmann::json();
    response->Result = doc.dump();
    return processer->SendResponse(handle, response);
}

void JRPCHookHelperAPI::OnConnectToHostRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = GetParamsNlohmannJson(*jreq);

    RecvConnectToHostDelegate(RPCHandle_t(req->GetID()), doc["processId"].get_ref<nlohmann::json::number_integer_t&>(), doc["commandline"].get_ref<nlohmann::json::string_t&>().c_str());
}

void JRPCHookHelperAPI::OnConnectToHostResponseRecv(std::shared_ptr<RPCResponse>resp, std::shared_ptr<RPCRequest>req)
{
    auto id = req->GetID();
    if (!HasConnectToHostSendDelagate(id)) {
        return;
    }
    std::shared_ptr<JsonRPCResponse> jresp = std::dynamic_pointer_cast<JsonRPCResponse>(resp);

    if (jresp->IsError()) {
        TriggerConnectToHostSendErrorDelegate(id, resp->ErrorCode, resp->ErrorMsg.c_str(), resp->ErrorData.c_str());
    }
    else {
        auto doc = jresp->GetResultNlohmannJson();
        TriggerConnectToHostSendDelegate(id);
    }
    RemoveConnectToHostSendDelagate(id);
}

REGISTER_RPC_API_AUTO(JRPCHookHelperAPI, AddWindow);
DEFINE_REQUEST_RPC(JRPCHookHelperAPI, AddWindow);
RPCHandle_t JRPCHookHelperAPI::AddWindow(uint64_t windowID, const char* sharedMemName, TAddWindowDelegate inDelegate, TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->SetMethod(AddWindowName);

    nlohmann::json obj = nlohmann::json::object();
    obj["windowID"] = windowID;
    obj["sharedMemName"] = sharedMemName;
    req->SetParams(obj.dump());

    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddAddWindowSendDelagate(req->GetID(), inDelegate, errDelegate);
    }
    return handle;
}

bool JRPCHookHelperAPI::RespondAddWindow(RPCHandle_t handle)
{
    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->OptError = false;

    nlohmann::json doc = nlohmann::json();
    response->Result = doc.dump();
    return processer->SendResponse(handle, response);
}

void JRPCHookHelperAPI::OnAddWindowRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = GetParamsNlohmannJson(*jreq);

    RecvAddWindowDelegate(RPCHandle_t(req->GetID()), doc["windowID"].get_ref<nlohmann::json::number_integer_t&>(), doc["sharedMemName"].get_ref<nlohmann::json::string_t&>().c_str());
}

void JRPCHookHelperAPI::OnAddWindowResponseRecv(std::shared_ptr<RPCResponse>resp, std::shared_ptr<RPCRequest>req)
{
    auto id = req->GetID();
    if (!HasAddWindowSendDelagate(id)) {
        return;
    }
    std::shared_ptr<JsonRPCResponse> jresp = std::dynamic_pointer_cast<JsonRPCResponse>(resp);

    if (jresp->IsError()) {
        TriggerAddWindowSendErrorDelegate(id, resp->ErrorCode, resp->ErrorMsg.c_str(), resp->ErrorData.c_str());
    }
    else {
        auto doc = jresp->GetResultNlohmannJson();
        TriggerAddWindowSendDelegate(id);
    }
    RemoveAddWindowSendDelagate(id);
}



REGISTER_RPC_API_AUTO(JRPCHookHelperAPI, RemoveWindow);
DEFINE_REQUEST_RPC(JRPCHookHelperAPI, RemoveWindow);
RPCHandle_t JRPCHookHelperAPI::RemoveWindow(uint64_t windowID,TRemoveWindowDelegate inDelegate, TRPCErrorDelegate errDelegate)
{
    std::shared_ptr<JsonRPCRequest> req = std::make_shared< JsonRPCRequest>();
    req->SetMethod(RemoveWindowName);

    nlohmann::json obj = nlohmann::json::object();
    obj["windowID"] = windowID;
    req->SetParams(obj.dump());

    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddRemoveWindowSendDelagate(req->GetID(), inDelegate, errDelegate);
    }
    return handle;
}

bool JRPCHookHelperAPI::RespondRemoveWindow(RPCHandle_t handle)
{
    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->OptError = false;

    nlohmann::json doc = nlohmann::json();
    response->Result = doc.dump();
    return processer->SendResponse(handle, response);
}

void JRPCHookHelperAPI::OnRemoveWindowRequestRecv(std::shared_ptr<RPCRequest> req)
{
    std::shared_ptr<JsonRPCRequest> jreq = std::dynamic_pointer_cast<JsonRPCRequest>(req);
    auto doc = GetParamsNlohmannJson(*jreq);

    RecvRemoveWindowDelegate(RPCHandle_t(req->GetID()), doc["windowID"].get_ref<nlohmann::json::number_integer_t&>());
}

void JRPCHookHelperAPI::OnRemoveWindowResponseRecv(std::shared_ptr<RPCResponse>resp, std::shared_ptr<RPCRequest>req)
{
    auto id = req->GetID();
    if (!HasRemoveWindowSendDelagate(id)) {
        return;
    }
    std::shared_ptr<JsonRPCResponse> jresp = std::dynamic_pointer_cast<JsonRPCResponse>(resp);

    if (jresp->IsError()) {
        TriggerRemoveWindowSendErrorDelegate(id, resp->ErrorCode, resp->ErrorMsg.c_str(), resp->ErrorData.c_str());
    }
    else {
        auto doc = jresp->GetResultNlohmannJson();
        TriggerRemoveWindowSendDelegate(id);
    }
    RemoveRemoveWindowSendDelagate(id);
}