#pragma once

#include <functional>

#include "simple_hook_helper_common.h"

class HOOK_HELPER_EXPORT IRPCHookHelperAPI
{
public:
    //typedef std::function<void(RPCHandle_t, uint32_t)> TConnectToGlobalUserDelegate;
    //virtual RPCHandle_t ConnectToGlobalUser(const char* appid, const char* token, TConnectToGlobalUserDelegate) = 0;
    //typedef std::function<void(RPCHandle_t, const char*, const char*)> TRecvConnectToGlobalUserDelegate;
    //virtual void RegisterConnectToGlobalUser(TRecvConnectToGlobalUserDelegate) = 0;
    //virtual bool RespondConnectToGlobalUser(RPCHandle_t id, uint32_t res) = 0;

    //typedef std::function<void(RPCHandle_t, bool)> TRuntimeInitDelegate;
    //virtual RPCHandle_t RuntimeInit(const char* host, uint16_t port, const char* dns_host, uint16_t dns_port, TRuntimeInitDelegate) = 0;
    //typedef std::function<void(RPCHandle_t, const char* , uint16_t , const char* , uint16_t )> TRecvRuntimeInitDelegate;
    //virtual void RegisterRuntimeInit(TRecvRuntimeInitDelegate) = 0;
    //virtual bool RespondRuntimeInit(RPCHandle_t handle, bool res) = 0;

    //typedef std::function<void(RPCHandle_t, bool)> TRuntimeUserStartDelegate;
    //virtual RPCHandle_t RuntimeUserStart(uint32_t user, const char* parameter, uint32_t reason, const char* region, TRuntimeUserStartDelegate) = 0;
    //typedef std::function<void(RPCHandle_t, uint32_t, const char* , uint32_t , const char* )> TRecvRuntimeUserStartDelegate;
    //virtual void RegisterRuntimeUserStart(TRecvRuntimeUserStartDelegate) = 0;
    //virtual bool RespondRuntimeUserStart(RPCHandle_t handle, bool res) = 0;

    //typedef std::function<void(RPCHandle_t, bool)> TRuntimeStopDelegate;
    //virtual RPCHandle_t RuntimeStop(TRuntimeStopDelegate) = 0;
    //typedef std::function<void(RPCHandle_t)> TRecvRuntimeStopDelegate;
    //virtual void RegisterRuntimeStop(TRecvRuntimeStopDelegate) = 0;
    //virtual bool RespondRuntimeStop(RPCHandle_t handle, bool res) = 0;

};