#pragma once

#include "rpc_interface.h"
#include "message_processer.h"
#include "delegate_macros.h"

#include <CharBuffer.h>
#include <std_ext.h>

#include <memory>
#include <set>
#include <unordered_map>
#include <optional>

class IGroupRPC;
class IRPCPaser;
class RPCRequest;
class RPCResponse;

class IPC_EXPORT IRPCProcesser {
public:

    virtual ~IRPCProcesser();

    typedef std::function<bool(std::shared_ptr<IRPCSerializable>, std::error_code&)> TSendRPCContentDelegate;
    virtual void Init(TSendRPCContentDelegate Delegate) {
        SendRPCContentDelegate = Delegate;
    }
    virtual RPCHandle_t SendRequest(IGroupRPC* group, std::shared_ptr<RPCRequest> request) =0;
    virtual bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) = 0;
    virtual bool SendEvent(IGroupRPC* group, std::shared_ptr<RPCRequest> request) = 0;

    virtual std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) = 0;

    virtual void OnRecevPacket(MessagePacket_t*, std::error_code&) = 0;
    virtual bool RecevReqInGroup(std::shared_ptr<IGroupRPC> group, std::shared_ptr<RPCRequest> req) {
        return group->OnRequestRecv(req);
    }
    virtual bool RecevRespInGroup(std::shared_ptr<IGroupRPC> group, std::shared_ptr<RPCResponse> resp, std::shared_ptr<RPCRequest> req) {
        return group->OnResponseRecv(resp, req);
    }

    bool AddGroupRPC(std::string_view groupName);
    template<class T>
    bool AddGroupRPC() {
        return AddGroupRPC(T::GetGroupName());
    }
    template<class T>
    T* GetInterface() {
        AddGroupRPC<T>();
        for (auto& pair : RPCAPIInterfaces) {
            T* result = dynamic_cast<T*>(pair.second.get());
            if (result) {
                return result;
            }
        }
        return nullptr;
    };
    std::shared_ptr<IGroupRPC> GetInterfaceByMethodName(const char* name);
    TSendRPCContentDelegate SendRPCContentDelegate;
    std::unordered_map<std::string_view, std::shared_ptr<IGroupRPC>,string_hash, std::equal_to<>> RPCAPIInterfaces;

};
class IPC_EXPORT FJRPCProcesser : public IRPCProcesser
{
public:
    FJRPCProcesser();
    ~FJRPCProcesser();

public:
    //send cancel is not thread safe
    RPCHandle_t SendRequest(IGroupRPC * group, std::shared_ptr<RPCRequest> request) override;
    std::shared_ptr<RPCRequest> CancelRequest(RPCHandle_t handle) override;
    bool SendEvent(IGroupRPC * group, std::shared_ptr<RPCRequest> request) override;
    bool SendResponse(RPCHandle_t handle, std::shared_ptr<RPCResponse> response) override;


    void OnRecevPacket(MessagePacket_t*, std::error_code&) override;


private:
    void OnRecevRPC(const char* str, uint32_t len, std::error_code&);

    std::atomic_uint32_t counter;


    std::unique_ptr<IRPCPaser> rpcParserInterface;
    std::unordered_map<RPCHandle_t, std::shared_ptr<RPCRequest>> requestMap;
};
