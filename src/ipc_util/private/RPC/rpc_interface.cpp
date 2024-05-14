#include "RPC/rpc_interface.h"
#include "RPC/rpc_processer.h"
std::unordered_map<std::string, RPCInterfaceInfo>* RPCInterfaceFactory::RPCInfos;
//static std::unordered_map<std::string, RPCInterfaceInfo>* RPCInfos;

RPCHandle_t IGroupRPC::SendRPCRequest(std::shared_ptr<RPCRequest> req)
{
    auto handle= processer->SendRequest(req);
    handle.PIGroupRPC = this;
    return handle;
}
std::shared_ptr<RPCRequest>  IGroupRPC::CancelRPCRequest(RPCHandle_t handle)
{
    return processer->CancelRequest(handle);
}

bool IGroupRPC::SendRPCResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response)
{
    return processer->SendResponse(handle, response);
}

bool RPCInterfaceFactory::Register(const char* name, RPCInterfaceInfo info)
{
    if (!RPCInfos) {
        RPCInfos = new std::unordered_map<std::string, RPCInterfaceInfo>();
    }
    auto it = RPCInfos->find(name);
    if (it == RPCInfos->end())
    {
        RPCInfos->emplace(name, info);
        //(*RPCInfos)[name] = info;
        return true;

    }
    return false;
}


std::unique_ptr<IGroupRPC> RPCInterfaceFactory::Create(const char* name, RPCProcesser* inprocesser, RPCInterfaceInfo::fnnew fn)
{
    if (auto it = RPCInfos->find(name); it != RPCInfos->end())
        return it->second.CreateFunc(inprocesser, fn); // call the createFunc

    return nullptr;
}
bool RPCInterfaceFactory::CheckMethod(const char* name, const char* methodname)
{
    if (auto it = RPCInfos->find(name); it != RPCInfos->end())
        return it->second.CheckFunc(methodname); // call the createFunc

    return false;
}

std::unordered_map<std::string, RPCInterfaceInfo>* RPCInterfaceFactory::GetRPCInfos()
{
    return RPCInfos;
}