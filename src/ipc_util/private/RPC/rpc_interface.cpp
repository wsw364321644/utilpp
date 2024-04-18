#include "RPC/rpc_interface.h"

std::unordered_map<std::string, RPCInterfaceInfo>* RPCInterfaceFactory::RPCInfos;
//static std::unordered_map<std::string, RPCInterfaceInfo>* RPCInfos;


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