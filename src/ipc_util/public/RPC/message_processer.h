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
    uint16_t ContentLength;
    uint8_t Channel;
    uint32_t Index;
};
class IPC_EXPORT  MessageProcesser
{
public:
    MessageProcesser();
    MessageProcesser(MessageSessionInterface*);
    ~MessageProcesser();

public:
    struct ConsumeResult_t {
        ConsumeResult_t();
        ~ConsumeResult_t();
        EMessageError Result;
        uint32_t ConsumptionLength;
        std::vector<std::shared_ptr<MessagePacket_t>> MessagePackets;
    };
    bool Init(MessageSessionInterface*);
    CharBuffer ChangePolicy(uint8_t channel, EMessagePolicy policy);
    bool SendContent(const char*, uint32_t len, uint8_t channel = 0);
    ConsumeResult_t TryConsume(const char* data, uint32_t len);

    /// <summary>
    /// invoke event in loop;
    /// </summary>
    void Tick();


public:
    EMessageConnectionType ConnectionType;
    DEFINE_EVENT_ONE_PARAM(OnPacketRecv, MessagePacket_t*)
private:

    CharBuffer BuildPacket(const char* data, uint32_t len, uint8_t channel = 0);
    void OnRead(MessageSessionInterface*, char*, ssize_t);
    void HeartBeat();
    void AddHeader(CharBuffer& buf, CharBuffer key, CharBuffer value);

private:

    MessageSessionInterface* session;
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