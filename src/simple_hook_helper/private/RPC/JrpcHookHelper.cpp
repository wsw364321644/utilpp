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
    req->Method = ConnectToHostName;

    nlohmann::json obj = nlohmann::json::object();
    obj["processId"] = processId;
    obj["commandline"] = commandline;
    req->Params = obj.dump();

    auto handle = processer->SendRequest(req);
    if (handle.IsValid()) {
        AddConnectToHostSendDelagate(req->ID.value(), inDelegate, errDelegate);
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
    auto doc = jreq->GetParamsNlohmannJson();

    recvConnectToHostDelegate(RPCHandle_t(req->ID.value()), doc["processId"].get_ref<nlohmann::json::number_integer_t&>(), doc["commandline"].get_ref<nlohmann::json::string_t&>().c_str());
}

void JRPCHookHelperAPI::OnConnectToHostResponseRecv(std::shared_ptr<RPCResponse>resp, std::shared_ptr<RPCRequest>req)
{
    auto id = req->ID.value();
    if (!HasConnectToHostSendDelagate(id)) {
        return;
    }
    std::shared_ptr<JsonRPCResponse> jresp = std::dynamic_pointer_cast<JsonRPCResponse>(resp);

    if (jresp->IsError()) {
        ConnectToHostErrorDelegates[id](RPCHandle_t(id), resp->ErrorCode, resp->ErrorMsg.c_str(), resp->ErrorData.c_str());
    }
    else {
        auto doc = jresp->GetResultNlohmannJson();
        ConnectToHostDelegates[id](RPCHandle_t(id));
    }
    RemoveConnectToHostSendDelagate(id);
}

