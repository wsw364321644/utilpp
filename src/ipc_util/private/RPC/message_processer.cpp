#include "RPC/message_processer.h"

#include <functional>
#include <format>

const uint32_t MAX_CHANNAL = 16;
const uint32_t MAX_CACHED_PACKET = 100;
const char* MessageBegin = "V1.0";
const char* MessageHeaderLength = "Length";
const char* MessageHeaderIndex = "Index";
const char* MessageHeaderChannel = "Channel";

const char* MessageHeaderSeparator = ":";
//const char* MessageCommand = "Command";
//const char* MessageCommandPolicy = "Policy";
//const char* MessageCommandHeartbeat = "Heartbeat";
const char* MessageEnd = "\r\n";
std::unordered_map<std::string, EMessageHeader> MessageProcesser::mapHeader{ {MessageHeaderLength,EMessageHeader::ContentLength}
,{MessageHeaderIndex,EMessageHeader::Index}
,{MessageHeaderChannel,EMessageHeader::Channel}
};
MessageProcesser::MessageProcesser() :sendChannels(MAX_CHANNAL, 0), recvChannels(MAX_CHANNAL, 0), ConnectionType(), session(nullptr), messageQueues(MAX_CHANNAL)
{
}
MessageProcesser::MessageProcesser(IMessageSession* mif) : MessageProcesser() {
    Init(mif);
}
MessageProcesser::~MessageProcesser()
{
}
bool MessageProcesser::Init(IMessageSession* insession)
{
    if (!insession) {
        return false;
    }
    session = insession;
    ConnectionType = session->GetConnectionType();

    session->AddOnReadDelegate(std::bind(&MessageProcesser::OnRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    /*if (ConnectionType == EMessageConnectionType::EMCT_UDP) {
        sendChannels.resize(std::numeric_limits<uint8_t>::max(), 0);
        recvChannels.resize(std::numeric_limits<uint8_t>::max(), 0);
    }*/
    lastHeartBeat = std::chrono::steady_clock::now();
    return true;
}
CharBuffer MessageProcesser::ChangePolicy(uint8_t channel, EMessagePolicy policy)
{
    return CharBuffer();
}

bool MessageProcesser::SendContent(const char* data, uint32_t len, uint8_t channel)
{
    auto buf = BuildPacket(data, len, channel);
    auto handle = session->Write(buf.CStr(), (int)buf.Length());
    return handle.IsValid();
}

MessageProcesser::ConsumeResult_t MessageProcesser::TryConsume(const char* data, uint32_t len)
{
    const char* ptr = data;
    uint32_t leftlen = len;
    ConsumeResult_t result;
    auto ConsumePtr = [&](uint32_t len) {
        ptr += len;
        leftlen -= len;
        };
    auto TryConsumeString = [&](const char* str)->bool {
        auto stringlen = strlen(str);
        if (leftlen < stringlen) {
            return false;
        }
        if (memcmp(str, ptr, stringlen)) {
            return false;
        }
        ConsumePtr(stringlen);
        return true;
        };
    auto ConsumeString = [&](const char* str)->bool {
        auto stringlen = strlen(str);
        if (leftlen < stringlen) {
            return false;
        }
        if (memcmp(str, ptr, stringlen)) {
            result.Result = EMessageError::ParseError;
            return false;
        }
        ConsumePtr(stringlen);
        return true;
        };
    auto FindHeader = [&](const char** outkey, int32_t* keylen, const char** outvalue, int32_t* valuelen)->bool {
        const char* headend = strstr(ptr, MessageEnd);
        if (headend == nullptr) {
            return false;
        }
        const char* headSeparator = strstr(ptr, MessageHeaderSeparator);
        if (headSeparator == nullptr || headSeparator >= headend) {
            result.Result = EMessageError::ParseError;
            return false;
        }
        *outkey = ptr;
        *keylen = headSeparator - ptr;
        *outvalue = headSeparator + strlen(MessageHeaderSeparator);
        *valuelen = headend - (headSeparator + strlen(MessageHeaderSeparator));
        ConsumePtr(headend + strlen(MessageEnd) - ptr);
        return true;
        };

    while (leftlen > 0) {
        uint32_t headleft = leftlen;
        auto packet = std::make_unique<MessagePacket_t>();
        if (!ConsumeString(MessageBegin)) {
            return result;
        }
        if (!ConsumeString(MessageEnd)) {
            return result;
        }
        while (!TryConsumeString(MessageEnd)) {
            const char* key = nullptr, * value = nullptr;
            int32_t keylen, valuelen;
            if (!FindHeader(&key, &keylen, &value, &valuelen)) {
                return result;
            }

            auto mapres = mapHeader.find(std::string(key, keylen));
            if (mapres == mapHeader.end()) {
                continue;
            }
            switch (mapres->second)
            {
            case EMessageHeader::ContentLength: {
                packet->ContentLength = std::stoi(std::string(value, valuelen));
                break;
            }
            case EMessageHeader::Channel: {
                packet->Channel = std::stoi(std::string(value, valuelen));
                break;
            }
            case EMessageHeader::Index: {
                packet->Index = std::stoi(std::string(value, valuelen));
                break;
            }
            default:
                break;
            }
        }
        if (packet->ContentLength > 0) {
            if (packet->ContentLength > leftlen) {
                return result;
            }
            packet->MessageContent = new(char[packet->ContentLength]);
            memcpy(packet->MessageContent, ptr, packet->ContentLength);
            ConsumePtr(packet->ContentLength);
        }
        result.ConsumptionLength += (headleft - leftlen);
        result.MessagePackets.emplace_back(std::move(packet));
    }
    return result;
}


void MessageProcesser::OnRead(IMessageSession* session, char* str, intptr_t size)
{
    if (ConnectionType == EMessageConnectionType::EMCT_UDP) {
        auto result = TryConsume(str, size);
        if (result.ConsumptionLength != size) {
            session->Disconnect();
            return;
        }

        for (auto const& packet : result.MessagePackets) {
            auto& queue = messageQueues[packet->Channel];
            if (channelsPolicy[packet->Channel] != EMessagePolicy::Unordered) {
                std::unique_lock<std::shared_mutex> lck(messageQueuesMutex);
                if (packet->Index - recvChannels[packet->Channel] > MAX_CACHED_PACKET) {
                    lck.unlock();
                    session->Disconnect();
                }

                auto itr = queue.rbegin();
                for (; itr != queue.rend(); itr++) {
                    if ((*itr)->Index == packet->Index) {
                        break;
                    }
                    if (packet->Index > (*itr)->Index ) {
                        queue.insert(++(itr.base()), packet);
                        break;
                    }
                }
                if (itr == queue.rend()) {
                    queue.insert(queue.begin(), packet);
                }

            }
            else {
                for (auto& packet : result.MessagePackets) {
                    std::unique_lock<std::shared_mutex> lck(messageQueuesMutex);
                    messageQueues[packet->Channel].push_back(std::move(packet));
                }
            }
        }


        for (int i = 0; i < MAX_CHANNAL; i++) {
            if (channelsPolicy[i] == EMessagePolicy::Unordered) {
                for (auto& packet : messageQueues[i]) {
                    TriggerOnPacketRecvDelegates(packet.get());
                }
                messageQueues[i].clear();
            }
            else {
                for (auto itr = messageQueues[i].begin(); itr != messageQueues[i].end();) {
                    if ((*itr)->Index != recvChannels[i]) {
                        break;
                    }
                    recvChannels[i]++;
                    TriggerOnPacketRecvDelegates((*itr).get());
                    itr = messageQueues[i].erase(itr);
                }
            }
        }
    }
    else {
        readBuf.insert(readBuf.end(), str, str + size);
        auto result = TryConsume(readBuf.data(), readBuf.size());
        if (result.Result != EMessageError::OK) {
            session->Disconnect();
            return;
        }
        readBuf = std::vector<decltype(readBuf)::value_type>(readBuf.begin() + result.ConsumptionLength, readBuf.end());

        for (auto& packet : result.MessagePackets) {
            std::unique_lock<std::shared_mutex> lck(messageQueuesMutex);
            messageQueues[packet->Channel].push_back(std::move(packet));
        }

        for (int i = 0; i < MAX_CHANNAL; i++) {
            for (auto& packet : messageQueues[i]) {
                TriggerOnPacketRecvDelegates(packet.get());
            }
            messageQueues[i].clear();
        }
    }
    return;
}
void MessageProcesser::HeartBeat()
{
    return;
}

CharBuffer MessageProcesser::BuildPacket(const char* data, uint32_t len, uint8_t channel)
{
    CharBuffer buff;
    buff.Append(MessageBegin, strlen(MessageBegin));
    buff.Append(MessageEnd, strlen(MessageEnd));
    if (channel > 0) {
        AddHeader(buff, MessageHeaderChannel, std::format("{}", channel).c_str());
    }
    AddHeader(buff, MessageHeaderIndex, std::format("{}", sendChannels[channel]++).c_str());

    if (len > 0) {
        AddHeader(buff, MessageHeaderLength, std::format("{}", len).c_str());
    }
    buff.Append(MessageEnd, strlen(MessageEnd));
    if (len > 0) {
        buff.Append(data, len);
    }
    return buff;
}

void MessageProcesser::AddHeader(CharBuffer& buf, CharBuffer key, CharBuffer value)
{
    buf.Append(key);
    buf.Append(MessageHeaderSeparator, strlen(MessageHeaderSeparator));
    buf.Append(value);
    buf.Append(MessageEnd, strlen(MessageEnd));
}

MessageProcesser::ConsumeResult_t::ConsumeResult_t() :ConsumptionLength(0), Result(EMessageError::OK)
{
}

MessageProcesser::ConsumeResult_t::~ConsumeResult_t()
{
}

MessagePacket_t::~MessagePacket_t()
{
    free(MessageContent);
}