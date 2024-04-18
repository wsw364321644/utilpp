#include "RPC/message_server.h"
#include "RPC/message_session.h"
#include "message_internal.h"
#include <logger.h>
#include <uv.h>
#include <uri.h>

#define TIMERREPEAT 1000 *60

MessageServerUV::MessageServerUV() :idCount(0), loop(nullptr)
{
    loop = (uv_loop_t*)malloc(sizeof * loop);
    uv_loop_init(loop);
}
MessageServerUV::~MessageServerUV()
{
    free(loop);
}

void MessageServerUV::Tick(float delSec)
{
    uv_run(loop, UV_RUN_NOWAIT);
}

void MessageServerUV::Run()
{
    uv_run(loop, UV_RUN_DEFAULT);
}


void MessageServerUV::Stop()
{
    uv_stop(loop);
    CloseServer();

    uv_walk(loop, [](uv_handle_t* handle, void* arg) {
        uv_close(handle, nullptr);
        }, nullptr);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_loop_close(loop);
}

void MessageServerUV::CloseConnection(MessageSessionUV* session)
{
    session->Disconnect();
}

bool MessageServerUV::OpenServer(EMessageConnectionType type, const std::string& inurl)
{
    messageConnectionType = type;

    std::string schemestr, authoritystr, portstr;
    url = inurl;
    ParsedURL_t parsedURL;
    parsedURL.outScheme = &schemestr;
    parsedURL.outAuthority = &authoritystr;
    parsedURL.outPort = &portstr;
    ParseUrl(url, &parsedURL);



    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {

        auto& pipeHandle = serverHandle.PipeHandle;
        if (uv_pipe_init(loop, &pipeHandle, 0) != 0) {
            return false;
        }
        pipeHandle.data = this;

        if (uv_pipe_bind(&pipeHandle, url.c_str()) != 0) {
            return false;
        }
        uv_pipe_chmod(&pipeHandle, UV_WRITABLE | UV_READABLE);
        if (uv_listen((uv_stream_t*)&pipeHandle, 128, UVCallBack::template UVOnConnection<MessageServerUV>) != 0) {
            return false;
        }
        break;
    }

    case EMessageConnectionType::EMCT_TCP: {
        auto port = std::stoi(portstr);
        if (port <= 0) {
            return false;
        }

        auto& tcpHandle = serverHandle.TCPHandle;
        if (uv_tcp_init(loop, &tcpHandle) != 0) {
            return false;
        }
        tcpHandle.data = this;
        struct sockaddr_in addr;
        if (uv_ip4_addr("0.0.0.0", port, &addr) != 0) {
            return false;
        }
        if (uv_tcp_bind(&tcpHandle, (const struct sockaddr*)&addr, 0) != 0) {
            return false;
        }
        if (uv_listen((uv_stream_t*)&tcpHandle, 128, UVCallBack::template UVOnConnection<MessageServerUV>) != 0) {
            return false;
        }
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        auto port = std::stoi(portstr);
        if (port <= 0) {
            return false;
        }
        auto& udpHandle = serverHandle.UDPHandle;
        if (uv_udp_init(loop, &udpHandle) != 0) {
            return false;
        }
        udpHandle.data = this;
        struct sockaddr_in addr;
        if (uv_ip4_addr("0.0.0.0", port, &addr) != 0) {
            return false;
        }
        uv_udp_bind(&udpHandle, (const struct sockaddr*)&addr, 0);
        uv_udp_recv_start(&udpHandle, UVCallBack::UVOnAlloc, UVCallBack::template UVOnUDPRecv<MessageServerUV>);
        break;
    }
    }

    return true;
}

void MessageServerUV::CloseServer()
{
    for (auto& session : sessions) {
        CloseConnection(session.get());
    }
    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        auto& pipeHandle = serverHandle.PipeHandle;
        uv_close((uv_handle_t*)&pipeHandle, NULL);
        break;
    }
    case EMessageConnectionType::EMCT_TCP: {
        auto& tcpHandle = serverHandle.TCPHandle;
        uv_close((uv_handle_t*)&tcpHandle, NULL);
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        auto& udpHandle = serverHandle.UDPHandle;
        uv_close((uv_handle_t*)&udpHandle, NULL);
        break;
    }
    }

}


void MessageServerUV::UVOnConnection(uv_stream_t* stream, int status)
{
    if (status < 0) {
        LOG_WARNING("{}, New connection error {}", Log_Server, uv_strerror(status));
        return;
    }

    MessageSessionUV* session;
    {
        std::scoped_lock<std::shared_mutex> guard(sessionMutex);
        sessions.emplace_back(std::make_unique<MessageSessionUV>(this));
        session = sessions.back().get();
    }
    session->ID = idCount++;

    switch (messageConnectionType) {
    case EMessageConnectionType::EMCT_IPC: {
        uv_pipe_t* client = &session->ClientHandle.PipeHandle;
        uv_pipe_init(loop, client, 0);
        if (uv_accept(stream, (uv_stream_t*)client) == 0) {
            uv_read_start((uv_stream_t*)client, UVCallBack::UVOnAlloc, UVCallBack::template UVOnRead<MessageSessionUV>);
        }
        else {
            CloseConnection(session);
            return;
        }
        break;

    }
    case EMessageConnectionType::EMCT_TCP: {
        uv_tcp_t* client = &session->ClientHandle.TCPHandle;
        uv_tcp_init(loop, client);
        if (uv_accept(stream, (uv_stream_t*)client) == 0) {
            uv_read_start((uv_stream_t*)client, UVCallBack::UVOnAlloc, UVCallBack::template UVOnRead<MessageSessionUV>);
        }
        else {
            CloseConnection(session);
            return;
        }
        break;
    }
    case EMessageConnectionType::EMCT_UDP: {
        LOG_ERROR("{},OnConnection udp recv other  error", Log_Server);
        break;
    }
    }

    TriggerOnConnectDelegates(session);
}



void MessageServerUV::UVOnUDPRecv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
{
    if (messageConnectionType != EMessageConnectionType::EMCT_UDP) {
        LOG_ERROR("{},OnConnection other recv udp error", Log_Server);
    }
    MessageSessionUV* session;
    sessionMutex.lock_shared();
    auto itr = std::find_if(sessions.begin(), sessions.end(), [&](const std::unique_ptr<MessageSessionUV>& ptr)->bool {
        return ptr->ClientHandle.RemoteAddr == addr;
        });

    if (itr == sessions.end()) {
        sessionMutex.unlock_shared();
        sessionMutex.lock();
        auto itr = std::find_if(sessions.begin(), sessions.end(), [&](const std::unique_ptr<MessageSessionUV>& ptr)->bool {
            return ptr->ClientHandle.RemoteAddr == addr;
            });
        if (itr == sessions.end()) {
            sessions.emplace_back(std::make_unique<MessageSessionUV>(this));
            session = sessions.back().get();
            session->ID = idCount++;
            sessionMutex.unlock();


            auto& client = session->ClientHandle.RemoteAddr;
            client = addr;

            TriggerOnConnectDelegates(session);
        }
        else {
            session = itr->get();
            sessionMutex.unlock();
        }
    }
    else {
        session = itr->get();
        sessionMutex.unlock_shared();
    }

    if (nread <= 0) {
        if (nread != UV_EOF) {
            LOG_WARNING("{}, Read from remote error: {}", Log_Server, nread);
        }
        session->Disconnect();
        return;
    }
    //session->ReadBuf.insert(session->ReadBuf.end(), buf->base, buf->base + nread);
    session->TriggerOnReadDelegates(session, buf->base, nread);
    UVCallBack::UVOnFree(buf);

}

void MessageServerUV::OnSessionClose(MessageSessionUV* session)
{
    session->TriggerOnDisconnectDelegates(session);
    {
        std::scoped_lock guard(sessionMutex);
        sessions.erase(std::remove_if(sessions.begin(), sessions.end(), [&](const auto& ptr)->bool {
            return  ptr.get() == session;
            }));
    }
}