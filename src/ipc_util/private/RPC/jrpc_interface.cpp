#include "RPC/jrpc_interface.h"
#include <jrpc_parser.h>

bool IGroupJRPC::RespondError(RPCHandle_t handle, int64_t errorCode, const char* errorMsg,const char* errorData)
{
    std::shared_ptr<JsonRPCResponse> response = std::make_shared< JsonRPCResponse>();
    response->SetError( true);
    response->ErrorCode = errorCode;
    if (errorMsg) {
        response->SetErrorMsg(errorMsg);
    }
    else {
        response->SetErrorMsg ("");
    }
    if (errorData) {
        response->SetErrorData (errorData);
    }
    return SendRPCResponse(handle, response);
}
