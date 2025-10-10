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
    IRPCProcesser(IMessageProcesser* processer);
    virtual ~IRPCProcesser();
    virtual bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) = 0;
    virtual RPCHandle_t SendRequest(IGroupRPC* group, std::shared_ptr<RPCRequest> request) = 0;
    virtual std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) = 0;
    virtual bool SendEvent(IGroupRPC* group, std::shared_ptr<RPCRequest> request) = 0;
    virtual void OnRecevPacket(MessagePacket_t*) =0;
    virtual bool RecevReqInGroup(std::shared_ptr<IGroupRPC> group, std::shared_ptr<RPCRequest> req) {
        return group->OnRequestRecv(req);
    }
    virtual bool RecevRespInGroup(std::shared_ptr<IGroupRPC> group, std::shared_ptr<RPCResponse> resp, std::shared_ptr<RPCRequest> req) {
        return group->OnResponseRecv(resp, req);
    }

    DEFINE_EVENT_ONE_PARAM(OnRPCConsumedError, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnMethoedNotFound, std::shared_ptr<RPCRequest>);
    DEFINE_EVENT_ONE_PARAM(OnRequestErrorRespond, std::shared_ptr<RPCResponse>);

    bool AddGroupRPC(std::string_view groupName);
    template<class T>
    bool AddGroupRPC() {
        return AddGroupRPC(T::GetGroupName());
    }
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

    std::unordered_map<std::string, std::shared_ptr<IGroupRPC>> RPCAPIInterfaces;
    IMessageProcesser* pMsgProcesser;
    CommonHandle_t OnPacketRecvHandle{ NullHandle };
};
class IPC_EXPORT FJRPCProcesser : public IRPCProcesser
{
public:
    FJRPCProcesser(IMessageProcesser*);
    ~FJRPCProcesser();

public:


    void OnRecevPacket(MessagePacket_t*) override;
    RPCHandle_t SendRequest(IGroupRPC* group,std::shared_ptr<RPCRequest> request) override;
    std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) override;
    bool SendEvent(IGroupRPC* group, std::shared_ptr<RPCRequest> request) override;
    bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) override;

private:
    void OnRecevRPC(const char* str, uint32_t len);

    std::atomic_uint32_t counter;


    std::unique_ptr<IRPCPaser> rpcParserInterface;
    std::unordered_map<RPCHandle_t, std::shared_ptr<RPCRequest>> requestMap;
    std::mutex requestMapMutex;


};
