#include "RPC/message_common.h"
#include "message_internal.h"

bool MessageSendRequestUV::Write(uv_stream_t* handle, const char* data, size_t len, OnWriteCB cb)
{
    Data.reset(new char[len]);
    Len = len;
    memcpy(Data.get(), data, Len);
    uv_buf_t buffers[1];
    buffers[0].base = Data.get(); 
    buffers[0].len = uint32_t(Len);
    SendReq.SendReq.data = this;
    WriteDelegate = cb;
    uv_write(&SendReq.SendReq, handle, buffers, 1, UVCallBack::template UVOnWrite<MessageSendRequestUV>);
    return true;
}
bool MessageSendRequestUV::UDPSend(uv_udp_t* handle, const char* data, size_t len, OnUDPSendCB cb)
{
    UDPSend(handle, data, len, NULL, cb);
    return true;
}
bool MessageSendRequestUV::UDPSend(uv_udp_t* handle, const char* data, size_t len, const sockaddr* addr, OnUDPSendCB cb)
{
    Data.reset(new char[len]);
    Len = len;
    memcpy(Data.get(), data, Len);
    uv_buf_t buffers[1];
    buffers[0].base = Data.get();
    buffers[0].len = uint32_t(Len);
    SendReq.UDPSendReq.data = this;
    WriteDelegate = cb;
    uv_udp_send(&SendReq.UDPSendReq, handle, buffers, 1, addr, UVCallBack::template UVOnUDPSend<MessageSendRequestUV>);
    return true;
}