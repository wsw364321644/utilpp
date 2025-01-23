#pragma once

#include <uv.h>
#include <regex>
#include <functional>
#include <variant>
#include <memory>
#include <cstring>
#include "delegate_macros.h"
#include "RPC/message_common.h"

class UVCallBack {
public:
    static void UVOnAlloc(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
        buf->base = (char*)malloc(size);
        buf->len = size;
    }

    static void UVOnFree(const uv_buf_t* buf) {
        free(buf->base);
    }

    template<class T>
    static void UVOnDNSResolved(uv_getaddrinfo_t* resolver, int status, struct addrinfo* res) {
        T* p = (T*)resolver->data;
        if (p) {
            p->UVOnDNSResolved(resolver, status, res);
        }
        //if (status < 0) {
        //    fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status));
        //    return;
        //}
        //char addr[17] = { '\0' };
        //uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
        //fprintf(stderr, "%s\n", addr);

        uv_freeaddrinfo(res);
    }


    //typedef std::function<void(uv_connect_t*, int)> UVOnConnectCB;
    template<class T>
    static void UVOnConnect(uv_connect_t* req, int status) {
        T* p = (T*)req->data;
        if (p) {
            p->UVOnConnect(req, status);
        }
    }

    //typedef std::function<void(uv_stream_t* , int )> UVOnConnectionCB;
    template<class T>
    static void UVOnConnection(uv_stream_t* server, int status) {
        T* p = (T*)server->data;
        if (p) {
            p->UVOnConnection(server, status);
        }
    }

    //typedef std::function<void(uv_stream_t*, ssize_t, const uv_buf_t*)> UVOnReadCB;
    template<class T>
    static void UVOnRead(uv_stream_t* req, ssize_t nread, const uv_buf_t* buf) {
        T* p = (T*)req->data;
        if (p) {
            p->UVOnRead(req, nread, buf);
        }
    }


    //typedef std::function<void(uv_write_t*, int)> UVOnWriteCB;
    template<class T>
    static void UVOnWrite(uv_write_t* req, int status) {
        T* p = (T*)req->data;
        if (p) {
            p->UVOnWrite(req, status);
        }
    }


    //typedef std::function<void(uv_udp_t*, ssize_t, const uv_buf_t*, const struct sockaddr*, unsigned)> UVOnUDPRecvCB;
    template<class T>
    static void UVOnUDPRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
        T* p = (T*)handle->data;
        if (p) {
            p->UVOnUDPRecv(handle, nread, buf, addr, flags);
        }
    }

    //typedef std::function<void(uv_udp_send_t*, int)> UVOnUDPSendCB;
    template<class T>
    static void UVOnUDPSend(uv_udp_send_t* req, int status) {
        T* p = (T*)req->data;
        if (p) {
            p->UVOnUDPSend(req, status);
        }
    }


    //typedef std::function<void(uv_shutdown_t* , int )> UVOnShutdownCB;
    template<class T>
    static void UVOnShutdown(uv_shutdown_t* req, int status) {
        T* p = (T*)req->data;
        if (p) {
            p->UVOnShutdown(req, status);
        }
    }

    //typedef std::function<void(uv_handle_t*)> UVOnCloseCB;
    template<class T>
    static void UVOnClose(uv_handle_t* handle) {
        T* p = (T*)handle->data;
        if (p) {
            p->UVOnClose(handle);
        }
    }

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

union UVRemoteHandle_u
{
    uv_pipe_t PipeHandle;
    NetworkAddress_t RemoteAddr;
    uv_tcp_t TCPHandle;
    UVRemoteHandle_u() :PipeHandle() {}
};


union UVHandle_u
{
    uv_pipe_t PipeHandle;
    uv_udp_t UDPHandle;
    uv_tcp_t TCPHandle;
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
    size_t Len{};
    CommonHandle_t Handle{ NullHandle };
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

    bool Write(uv_stream_t* handle, const char* data, size_t len, OnWriteCB cb);
    bool UDPSend(uv_udp_t* handle, const char* data, size_t len, OnUDPSendCB cb);
    bool UDPSend(uv_udp_t* handle, const char* data, size_t len, const sockaddr* addr, OnUDPSendCB cb);
private:
    void UVOnWrite(uv_write_t* req, int status) {
        std::get<OnWriteCB>(WriteDelegate)(this, req, status);
    }
    void UVOnUDPSend(uv_udp_send_t* req, int status) {
        std::get<OnUDPSendCB>(WriteDelegate)(this, req, status);
    }
};

