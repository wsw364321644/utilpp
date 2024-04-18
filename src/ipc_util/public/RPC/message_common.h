#pragma once

#include <uv.h>
#include <regex>
#include <functional>
#include <variant>
#include <memory>
#include <vector>
#include "ipc_util_export.h"
#include "delegate_macros.h"


#pragma warning(push)
#pragma warning(disable:4251)


enum class EMessageConnectionType {
    EMCT_TCP,
    EMCT_IPC,
    EMCT_UDP
};
enum class EMessageError {
    OK,
    ParseError,
    InternalError
};

union UVHandle_u
{
    uv_pipe_t PipeHandle;
    uv_udp_t UDPHandle;
    uv_tcp_t TCPHandle;
};


struct NetworkAddress_t
{
    using addr_variant = std::variant<sockaddr_in, sockaddr_in6>;
    addr_variant Addr;
    NetworkAddress_t() = default;
    NetworkAddress_t(const NetworkAddress_t& inaddr) = default;
    explicit NetworkAddress_t(const addrinfo* inaddr)
    {
        switch (inaddr->ai_family) {
        case AF_INET:
            this->Addr = reinterpret_cast<const sockaddr_in&>(*inaddr->ai_addr);
            break;
        case AF_INET6:
            this->Addr = reinterpret_cast<const sockaddr_in6&>(*inaddr->ai_addr);
            break;
        default:
            throw std::runtime_error("Unsupported network address");
        }
    }
    explicit NetworkAddress_t(sockaddr* inaddr)
    {
        switch (inaddr->sa_family) {
        case AF_INET:
            this->Addr = reinterpret_cast<const sockaddr_in&>(*inaddr);
            break;
        case AF_INET6:
            this->Addr = reinterpret_cast<const sockaddr_in6&>(*inaddr);
            break;
        default:
            throw std::runtime_error("Unsupported network address");
        }
    }
    bool operator==(const sockaddr*& inaddr) {
        if (inaddr->sa_family != GetAdddr()->sa_family) {
            return false;
        }
        switch (inaddr->sa_family) {
        case AF_INET:
            return memcmp(&std::get<sockaddr_in>(Addr), inaddr, sizeof(sockaddr_in)) == 0;
            break;
        case AF_INET6:
            return memcmp(&std::get<sockaddr_in6>(Addr), inaddr, sizeof(sockaddr_in6)) == 0;
            break;
        default:
            return false;
        }
    }
    NetworkAddress_t& operator=(const sockaddr*& inaddr) {
        switch (inaddr->sa_family) {
        case AF_INET:
            this->Addr = reinterpret_cast<const sockaddr_in&>(*inaddr);
            break;
        case AF_INET6:
            this->Addr = reinterpret_cast<const sockaddr_in6&>(*inaddr);
            break;
        default:
            throw std::runtime_error("Unsupported network address");
        }
        return *this;
    }
    std::size_t AddrLen() const noexcept
    {
        switch (Addr.index()) {
        case 0: return sizeof(sockaddr_in);
        case 1: return sizeof(sockaddr_in6);
        default: return 0;
        }
    }
    const sockaddr* GetAdddr() {
        switch (Addr.index()) {
        case 0:
            return reinterpret_cast<const sockaddr*>(&std::get<0>(Addr));
            break;
        case 1:
            return reinterpret_cast<const sockaddr*>(&std::get<1>(Addr));
            break;
        default:
            throw std::bad_variant_access{};
        }
    }
    int connect(int socket) const
    {
#     if __cpp_generic_lambdas >= 201707L
        /* C++20: Use templated lambda */
        return std::visit([socket]<class T>(const T & addr) noexcept -> int {
            return ::connect(socket,
                reinterpret_cast<const sockaddr*>(&addr),
                sizeof(T));
        }, Addr);
#     else
        const sockaddr* ptr;
        int addrlen = 0;
        switch (Addr.index()) {
        case 0:
            ptr = reinterpret_cast<const sockaddr*>(&std::get<0>(Addr));
            addrlen = sizeof(sockaddr_in);
            break;
        case 1:
            ptr = reinterpret_cast<const sockaddr*>(&std::get<1>(Addr));
            addrlen = sizeof(sockaddr_in6);
            break;
        default:
            throw std::bad_variant_access{};
        }
        return ::connect(socket, ptr, addrlen);
#     endif
    }
};

union IPC_EXPORT UVRemoteHandle_u
{
    uv_pipe_t PipeHandle;
    NetworkAddress_t RemoteAddr;
    uv_tcp_t TCPHandle;
    UVRemoteHandle_u():PipeHandle(){}
};


class MessageSendRequestUV {
    friend class UVCallBack;
public:
    typedef std::function<void(MessageSendRequestUV*, uv_write_t* req, int status)> OnWriteCB;
    typedef std::function<void(MessageSendRequestUV*, uv_udp_send_t* req, int status)> OnUDPSendCB;
    std::variant< OnWriteCB, OnUDPSendCB> WriteDelegate;


    union
    {
        uv_udp_send_t UDPSendReq;
        uv_write_t SendReq;
    }SendReq{};
    std::unique_ptr<char> Data;
    int Len{};
    CommonHandle_t Handle;
    EMessageConnectionType  ConnectionType;
    MessageSendRequestUV(const MessageSendRequestUV&& sendhandle)noexcept : ConnectionType(sendhandle.ConnectionType) {
        switch (sendhandle.ConnectionType) {
        case EMessageConnectionType::EMCT_IPC:
        case EMessageConnectionType::EMCT_TCP: {
            WriteDelegate = sendhandle.WriteDelegate;
            break;
        }
        case EMessageConnectionType::EMCT_UDP: {
            WriteDelegate = sendhandle.WriteDelegate;
            break;
        }
        }
    }
    MessageSendRequestUV(EMessageConnectionType t) noexcept : ConnectionType(t) {

    }

    bool Write(uv_stream_t* handle, const char* data, int len, OnWriteCB cb);
    bool UDPSend(uv_udp_t* handle, const char* data, int len, OnUDPSendCB cb);
    bool UDPSend(uv_udp_t* handle, const char* data, int len, const sockaddr* addr, OnUDPSendCB cb);
private:
    void UVOnWrite(uv_write_t* req, int status) {
        std::get<OnWriteCB>(WriteDelegate)(this, req, status);
    }
    void UVOnUDPSend(uv_udp_send_t* req, int status) {
        std::get<OnUDPSendCB>(WriteDelegate)(this, req, status);
    }
};





enum class EMessageConnectionState :uint8_t {
    Idle,
    InitConnect,
    Connecting,
    Connected,
    WaitClose,
    Closing
};

class IPC_EXPORT MessageSessionInterface {
public:
    virtual EMessageConnectionType GetConnectionType()const = 0;
    virtual void Disconnect() = 0;
    virtual CommonHandle_t Write(const char* data, int len) = 0;
    virtual EMessageConnectionState GetConnectionState()const = 0;
    DEFINE_EVENT_ONE_PARAM(OnDisconnect, MessageSessionInterface*);
    DEFINE_EVENT_THREE_PARAM(OnRead, MessageSessionInterface*, char*, ssize_t);
    DEFINE_EVENT_TWO_PARAM(OnWrite, MessageSessionInterface*, MessageSendRequestUV*);
};

#pragma warning(pop)