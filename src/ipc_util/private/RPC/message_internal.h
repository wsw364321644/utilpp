#pragma once

#include <uv.h>
#include <regex>
#include <functional>
#include <variant>
#include <memory>
#include "delegate_macros.h"

class UVCallBack {
public:
    static void UVOnAlloc(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
        buf->base = (char*)malloc(size);
        buf->len = (ULONG)size;
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
