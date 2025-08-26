#pragma once
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <map>
#include <string>
#include <chrono>
#include <list>
#include <string_buffer.h>
#include "message_common.h"
#include "delegate_macros.h"
#pragma warning(push)
#pragma warning(disable:4251)


enum class EMessagePolicy :uint8_t {
    Unordered,
    Ordered,
    Stream
};
class EMessagePolicyInfo {
public:
    static const char* ToString(EMessagePolicy policy) {
        switch (policy) {
        case EMessagePolicy::Unordered:
            return "Unordered";
        case EMessagePolicy::Ordered:
            return "Ordered";
        case EMessagePolicy::Stream:
            return "Stream";
        }
        return "";
    }
};

enum class EMessageHeader :uint8_t {
    ContentLength,
    Channel,
    Index,
    Custom
};


struct IPC_EXPORT MessagePacket_t {
    MessagePacket_t() = default;
    MessagePacket_t(const MessagePacket_t&) = delete;
    ~MessagePacket_t();
    char* MessageContent;
    uint32_t ContentLength;
    uint8_t Channel;
    uint32_t Index;
};



class IPC_EXPORT IMessageProcesser {
public:
    typedef struct ConsumeResult_t {
        ConsumeResult_t();
        ~ConsumeResult_t();
        EMessageError Result;
        uint32_t ConsumptionLength;
        std::vector<std::shared_ptr<MessagePacket_t>> MessagePackets;
    }ConsumeResult_t;

    virtual ~IMessageProcesser() = default;
    virtual FCharBuffer ChangePolicy(uint8_t channel, EMessagePolicy policy) =0;
    virtual bool SendContent(const char*, uint32_t len, uint8_t channel = 0) =0;
    virtual ConsumeResult_t TryConsume(const char* data, uint32_t len) =0;

    DEFINE_EVENT_ONE_PARAM(OnPacketRecv, MessagePacket_t*)
};


class IPC_EXPORT FMessageProcesser:public IMessageProcesser
{
public:
    FMessageProcesser();
    FMessageProcesser(IMessageSession*);
    ~FMessageProcesser();

public:
    bool Init(IMessageSession*);
    FCharBuffer ChangePolicy(uint8_t channel, EMessagePolicy policy) override;
    bool SendContent(const char*, uint32_t len, uint8_t channel = 0) override;
    ConsumeResult_t TryConsume(const char* data, uint32_t len) override;

public:
    EMessageConnectionType ConnectionType;

private:

    FCharBuffer BuildPacket(const char* data, uint32_t len, uint8_t channel = 0);
    void OnRead(IMessageSession*, char*, intptr_t);
    void HeartBeat();
    void AddHeader(FCharBuffer& buf, FCharBuffer key, FCharBuffer value);

private:

    IMessageSession* session;
    CommonHandle_t MessageSessionOnReadHandle{NullHandle};
    std::vector<char> readBuf;
    std::vector<std::list<std::shared_ptr<MessagePacket_t>>> messageQueues;
    std::shared_mutex messageQueuesMutex;
    std::vector<EMessagePolicy> channelsPolicy;
    std::vector<uint32_t> sendChannels;
    std::vector<uint32_t> recvChannels;
    std::chrono::time_point<std::chrono::steady_clock> lastHeartBeat;
    static std::unordered_map<std::string, EMessageHeader> mapHeader;
};
#pragma warning(pop)