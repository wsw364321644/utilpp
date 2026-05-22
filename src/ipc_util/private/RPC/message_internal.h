#pragma once

#include <uv.h>
#include <regex>
#include <functional>
#include <variant>
#include <memory>
#include <cstring>
#include <SimpleIPAddress.h>
#include "delegate_macros.h"
#include "RPC/message_common.h"

class UVCallBack {
public:
    static void UVOnAlloc(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
        buf->base = (char*)malloc(size);
#ifdef _WIN32
        buf->len = (ULONG)size;
#else
        buf->len = size;
#endif // _WIN32

        
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

union UVRemoteHandle_u
{
    uv_pipe_t PipeHandle;
    FSimpleIPAddress RemoteAddr;
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
    CommonHandle32_t Handle{ NullHandle };
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

