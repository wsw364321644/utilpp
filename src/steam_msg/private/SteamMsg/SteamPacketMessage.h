#pragma once
#include <stdint.h>
#include <SteamTypes.h>
#include <SteamLanguage.h>
#include <steam/steammessages_base.pb.h>

constexpr uint32_t ProtoMask = 0x80000000;
class ISteamPacketMsg {
public:
    virtual ~ISteamPacketMsg() = default;
    virtual std::tuple< std::string_view, bool> Parse(const char* data, int32_t len) = 0;

    virtual uint64_t GetTargetJobID() const = 0;
    virtual uint64_t GetSourceJobID() const = 0;
    virtual utilpp::steam::EMsg GetMsgType() const = 0;
    virtual void SetMsgType(utilpp::steam::EMsg) = 0;
    virtual bool IsProtoBuf() const = 0;
    virtual void SetProtoBuf(bool bProtoBuf) = 0;
};

typedef struct MsgHeader_t {
    uint64_t TargetJobID;
    uint64_t SourceJobID;
}MsgHeader_t;

typedef struct ExtendedClientMsgHeader_t {
    uint8_t HeaderSize;
    uint16_t HeaderVersion;
    uint64_t TargetJobID;
    uint64_t SourceJobID;
    uint8_t HeaderCanary;
    uint64_t SteamID;
    int32_t SessionID;
}ExtendedClientMsgHeader_t;



class FSteamPacketMsg :public ISteamPacketMsg {
public:

    std::tuple<std::string_view, bool> Parse(const char* data, int32_t len) override;
    std::tuple<std::string_view, bool> SerializeToOstream(uint32_t bodyLen, std::function<bool(std::ostream*)> bodySerializeFunc);
    utilpp::steam::EMsg GetMsgType() const override {
        return MsgType;
    }
    void SetMsgType(utilpp::steam::EMsg inMsgType) override {
        MsgType = inMsgType;
    }
    bool IsProtoBuf() const override {
        return bProtoBuf;
    }
    void SetProtoBuf(bool _bProtoBuf) {
        bProtoBuf = _bProtoBuf;
    }
    uint64_t GetTargetJobID() const override {
        auto pMsgHeader = std::get_if<MsgHeader_t>(&Header);
        auto pProtoBufHeader = std::get_if<utilpp::steam::CMsgProtoBufHeader>(&Header);
        auto pExtendedClientMsgHeader = std::get_if<ExtendedClientMsgHeader_t>(&Header);
        if (pProtoBufHeader) {
            return pProtoBufHeader->jobid_target();
        }
        else if (pExtendedClientMsgHeader) {
            return pExtendedClientMsgHeader->TargetJobID;
        }
        else {
            return pMsgHeader->TargetJobID;
        }
    }
    uint64_t GetSourceJobID() const override {
        auto pMsgHeader = std::get_if<MsgHeader_t>(&Header);
        auto pProtoBufHeader = std::get_if<utilpp::steam::CMsgProtoBufHeader>(&Header);
        auto pExtendedClientMsgHeader = std::get_if<ExtendedClientMsgHeader_t>(&Header);
        if (pProtoBufHeader) {
            return pProtoBufHeader->jobid_source();
        }
        else if (pExtendedClientMsgHeader) {
            return pExtendedClientMsgHeader->SourceJobID;
        }
        else {
            return pMsgHeader->SourceJobID;
        }
    }

    utilpp::steam::EMsg MsgType;
    bool bProtoBuf;
    std::variant<utilpp::steam::CMsgProtoBufHeader, MsgHeader_t, ExtendedClientMsgHeader_t>Header;


private:
    //for Serialize
    std::vector<uint8_t> Buf;
};
