#pragma once
#include <system_error>
#include <delegate_macros.h>
#include <variant>
#include "ws_export_defs.h"
class IWebsocketEndpoint
{
public:
    virtual std::variant<uint32_t, std::string> GetLocalIP() = 0;
    virtual void SetSSl(bool) = 0;
    virtual void SetPortNum(int) = 0;
    virtual void SendData(const char*,size_t) = 0;
    virtual const std::error_code& GetLastError() const=0;
};