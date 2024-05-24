#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>


#include "rpc_interface.h"
#include "message_processer.h"
#include "delegate_macros.h"


class IGroupRPC;
class IRPCPaser;
class RPCRequest;
class RPCResponse;
class IPC_EXPORT RPCProcesser
{
public:
    RPCProcesser(MessageProcesser*);
    ~RPCProcesser();

public:
    template<class T>
    T* GetInterface() {
        //T::GetGroupName();
        for (auto& pair : RPCAPIInterfaces) {
            T* result = dynamic_cast<T*>(pair.second.get());
            if (result) {
                return result;
            }
        }
        return nullptr;
    };
    std::shared_ptr<IGroupRPC> GetInterfaceByMethodName(const char* name);
    void OnRecevRPC(const char* str, uint32_t len);
    void OnRecevPacket(MessagePacket_t*);

    RPCHandle_t SendRequest(std::shared_ptr<RPCRequest> request);
    std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle);
    bool SendEvent(std::shared_ptr<RPCRequest> request);
    bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response);

    DEFINE_EVENT_ONE_PARAM(OnRPCConsumedError, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnMethoedNotFound, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnRequestErrorRespond, std::shared_ptr<RPCResponse>);
private:

    MessageProcesser* msgprocesser;
    std::atomic_uint32_t counter;

    //std::unique_ptr<IRPCCommon> rpcCommonInterface;
    //std::unique_ptr<IRPCCommand> rpcCommandInterface;
    //static std::unordered_map<std::string, std::shared_ptr<IGroupRPC>> RPCAPIInterfaces;
    std::unordered_map<std::string, std::shared_ptr<IGroupRPC>> RPCAPIInterfaces;
    std::unique_ptr<IRPCPaser> rpcParserInterface;
    std::unordered_map<RPCHandle_t, std::shared_ptr<RPCRequest>> requestMap;
    std::mutex requestMapMutex;
};
