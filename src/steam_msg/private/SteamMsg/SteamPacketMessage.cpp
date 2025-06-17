#include "SteamMsg/SteamPacketMessage.h"
#include <spanstream>


std::tuple< std::string_view, bool> FSteamPacketMsg::Parse(const char* data, int32_t len)
{
    if (len < sizeof(uint32_t)) {
        return { "",false };
    }
    std::span<const char> inSpan(data, len);
    std::ispanstream stream(inSpan);
    size_t readLen{ 0 };
    stream.read((char*)&MsgType, sizeof(int32_t));
    readLen += stream.gcount();
    bProtoBuf = std::to_underlying(MsgType) & ProtoMask;
    MsgType = EMsg(std::to_underlying(MsgType) & ~ProtoMask);

    switch (MsgType)
    {
    case EMsg::ChannelEncryptRequest:
    case EMsg::ChannelEncryptResponse:
    case EMsg::ChannelEncryptResult: {
        Header = MsgHeader_t();
        auto& MsgHeader = std::get<MsgHeader_t>(Header);
        stream.read((char*)&MsgHeader.TargetJobID, sizeof(MsgHeader.TargetJobID));
        readLen += stream.gcount();
        stream.read((char*)&MsgHeader.SourceJobID, sizeof(MsgHeader.SourceJobID));
        readLen += stream.gcount();
        break;
    }
    default: {
        if (bProtoBuf) {
            int32_t ProtoBufHeaderLen;
            stream.read((char*)&ProtoBufHeaderLen, sizeof(ProtoBufHeaderLen));
            readLen += stream.gcount();
            Header = utilpp::steam::CMsgProtoBufHeader();
            auto& ProtoBufHeader = std::get<utilpp::steam::CMsgProtoBufHeader>(Header);
            auto bres = ProtoBufHeader.ParseFromIstream(&stream);
            if (!bres) {
                return { "",false };
            }
            readLen += ProtoBufHeaderLen;

        }
        else {
            Header = ExtendedClientMsgHeader_t();
            auto& ExtendedClientMsgHeader = std::get<ExtendedClientMsgHeader_t>(Header);
            stream.read((char*)&ExtendedClientMsgHeader.HeaderSize, sizeof(ExtendedClientMsgHeader.HeaderSize));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.HeaderVersion, sizeof(ExtendedClientMsgHeader.HeaderVersion));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.TargetJobID, sizeof(ExtendedClientMsgHeader.TargetJobID));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.SourceJobID, sizeof(ExtendedClientMsgHeader.SourceJobID));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.HeaderCanary, sizeof(ExtendedClientMsgHeader.HeaderCanary));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.steamID, sizeof(ExtendedClientMsgHeader.steamID));
            readLen += stream.gcount();
            stream.read((char*)&ExtendedClientMsgHeader.SessionID, sizeof(ExtendedClientMsgHeader.SessionID));
            readLen += stream.gcount();
        }
    }
    }
    auto BodyLen = len - readLen;
    return { std::string_view(data + readLen, BodyLen),true };
}

std::tuple<std::string_view, bool> FSteamPacketMsg::SerializeToOstream(uint32_t bodyLen, std::function<bool(std::ostream*)> bodySerializeFunc)
{
    auto pMsgHeader = std::get_if<MsgHeader_t>(&Header);
    auto pProtoBufHeader = std::get_if<utilpp::steam::CMsgProtoBufHeader>(&Header);
    auto pExtendedClientMsgHeader = std::get_if<ExtendedClientMsgHeader_t>(&Header);
    switch (MsgType)
    {
    case EMsg::ChannelEncryptRequest:
    case EMsg::ChannelEncryptResponse:
    case EMsg::ChannelEncryptResult: {
        if (!pMsgHeader) {
            return { "",false };
        }
        Buf.reserve(sizeof(int32_t) + sizeof(MsgHeader_t) + bodyLen);
        break;
    }
    default: {
        if (IsProtoBuf()) {
            if (!pProtoBufHeader) {
                return { "",false };
            }
            auto& ProtoBufHeader = *pProtoBufHeader;
            Buf.reserve(sizeof(int32_t) + sizeof(int32_t) + ProtoBufHeader.ByteSizeLong() + bodyLen);
        }
        else {
            if (!pExtendedClientMsgHeader) {
                return { "", false };
            }
            Buf.reserve(sizeof(int32_t) + sizeof(ExtendedClientMsgHeader_t) + bodyLen);
        }
    }
    }
    std::span<char> inspan((char*)Buf.data(), Buf.capacity());
    std::ospanstream stream(inspan);

    switch (MsgType)
    {
    case EMsg::ChannelEncryptRequest:
    case EMsg::ChannelEncryptResponse:
    case EMsg::ChannelEncryptResult: {
        stream.write((char*)&MsgType, sizeof(int32_t));
        auto& MsgHeader = *pMsgHeader;
        stream.write((char*)&MsgHeader.TargetJobID, sizeof(MsgHeader.TargetJobID));
        stream.write((char*)&MsgHeader.SourceJobID, sizeof(MsgHeader.SourceJobID));
        break;
    }
    default: {
        if (IsProtoBuf()) {
            auto ProtoMsgType = std::to_underlying(MsgType) | ProtoMask;
            stream.write((char*)&ProtoMsgType, sizeof(int32_t));
            auto& ProtoBufHeader = *pProtoBufHeader;
            int32_t HeaderLen = ProtoBufHeader.ByteSizeLong();
            stream.write((char*)&HeaderLen, sizeof(HeaderLen));
            ProtoBufHeader.SerializeToOstream(&stream);
        }
        else {
            stream.write((char*)&MsgType, sizeof(int32_t));
            auto& ExtendedClientMsgHeader = *pExtendedClientMsgHeader;
            stream.write((char*)&ExtendedClientMsgHeader.HeaderSize, sizeof(ExtendedClientMsgHeader.HeaderSize));
            stream.write((char*)&ExtendedClientMsgHeader.HeaderVersion, sizeof(ExtendedClientMsgHeader.HeaderVersion));
            stream.write((char*)&ExtendedClientMsgHeader.TargetJobID, sizeof(ExtendedClientMsgHeader.TargetJobID));
            stream.write((char*)&ExtendedClientMsgHeader.SourceJobID, sizeof(ExtendedClientMsgHeader.SourceJobID));
            stream.write((char*)&ExtendedClientMsgHeader.HeaderCanary, sizeof(ExtendedClientMsgHeader.HeaderCanary));
            stream.write((char*)&ExtendedClientMsgHeader.steamID, sizeof(ExtendedClientMsgHeader.steamID));
            stream.write((char*)&ExtendedClientMsgHeader.SessionID, sizeof(ExtendedClientMsgHeader.SessionID));
        }
    }
    }
    bodySerializeFunc(&stream);
    return { std::string_view((const char*)Buf.data(),stream.tellp()),true };
}

