#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>
#include <string_buffer.h>

#include "rpc_interface.h"
#include "message_processer.h"
#include "delegate_macros.h"


class IGroupRPC;
class IRPCPaser;
class RPCRequest;
class RPCResponse;

class IPC_EXPORT IRPCProcesser {
public:
    virtual ~IRPCProcesser() = default;
    virtual bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) = 0;
    virtual RPCHandle_t SendRequest(std::shared_ptr<RPCRequest> request) = 0;
    virtual std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) = 0;
    virtual bool SendEvent(std::shared_ptr<RPCRequest> request) = 0;

    DEFINE_EVENT_ONE_PARAM(OnRPCConsumedError, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnMethoedNotFound, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnRequestErrorRespond, std::shared_ptr<RPCResponse>);
};
class IPC_EXPORT FJRPCProcesser : public IRPCProcesser
{
public:
    FJRPCProcesser(IMessageProcesser*);
    ~FJRPCProcesser();

public:
    bool AddGroupRPC(std::string_view groupName);
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

    RPCHandle_t SendRequest(std::shared_ptr<RPCRequest> request) override;
    std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) override;
    bool SendEvent(std::shared_ptr<RPCRequest> request) override;
    bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) override;

private:

    IMessageProcesser* msgprocesser;
    std::atomic_uint32_t counter;

    //std::unique_ptr<IRPCCommon> rpcCommonInterface;
    //std::unique_ptr<IRPCCommand> rpcCommandInterface;
    //static std::unordered_map<std::string, std::shared_ptr<IGroupRPC>> RPCAPIInterfaces;
    std::unordered_map<std::string, std::shared_ptr<IGroupRPC>> RPCAPIInterfaces;
    std::unique_ptr<IRPCPaser> rpcParserInterface;
    std::unordered_map<RPCHandle_t, std::shared_ptr<RPCRequest>> requestMap;
    std::mutex requestMapMutex;
    CommonHandle_t OnPacketRecvHandle{NullHandle};

};
