#pragma once
#include "SteamLanguage.h"
#include <bit_vector.h>
class FSteamID {
public:
    static constexpr char UnknownAccountTypeChar = 'i';
    static constexpr const uint32_t AllInstances = 0;
    static constexpr const uint32_t DesktopInstance = 1;
    static constexpr const uint32_t ConsoleInstance = 2;
    static constexpr const uint32_t WebInstance = 4;
    static constexpr const uint32_t AccountIDMask = 0xFFFFFFFF;
    static constexpr const uint32_t AccountInstanceMask = 0x000FFFFF;

    FSteamID() = default;
    FSteamID(uint64_t val) {
        SetValue(val);
    }
    FSteamID(const FSteamID& inID) {
        Value = inID.Value;
    }

    void SetAccountID(uint32_t val) {
        Value.SetBitSet(val, AccountIDBitOffset, AccountIDBitLen);
    }
    uint32_t GetAccountID() const {
        return Value.template GetBitSet<uint32_t>(AccountIDBitOffset, AccountIDBitLen);
    }
    void SetAccountInstance(uint32_t val) {
        Value.SetBitSet(val, AccountInstanceBitOffset, AccountInstanceBitLen);
    }
    uint32_t GetAccountInstance() const {
        return Value.template GetBitSet<uint32_t>(AccountInstanceBitOffset, AccountInstanceBitLen);
    }
    void SetAccountUniverse(utilpp::steam::EUniverse val) {
        Value.SetBitSet(std::to_underlying(val), AccountUniverseBitOffset, AccountUniverseBitLen);
    }
    utilpp::steam::EUniverse GetAccountUniverse() const {
        return Value.template GetBitSet<utilpp::steam::EUniverse>(AccountUniverseBitOffset, AccountUniverseBitLen);
    }
    void SetAccountType(utilpp::steam::EAccountType val) {
        Value.SetBitSet(std::to_underlying(val), AccountTypeBitOffset, AccountTypeBitLen);
    }
    utilpp::steam::EAccountType GetAccountType() const {
        return Value.template GetBitSet<utilpp::steam::EAccountType>(AccountTypeBitOffset, AccountTypeBitLen);
    }
    bool Isvalis() {
        if (GetAccountType() <= utilpp::steam::EAccountType::Invalid || GetAccountType() > utilpp::steam::EAccountType::AnonUser)
            return false;

        if (GetAccountUniverse() <= utilpp::steam::EUniverse::Invalid || GetAccountUniverse() > utilpp::steam::EUniverse::Dev)
            return false;

        if (GetAccountType() == utilpp::steam::EAccountType::Individual)
        {
            if (GetAccountID() == 0 || GetAccountInstance() > WebInstance)
                return false;
        }

        if (GetAccountType() == utilpp::steam::EAccountType::Clan)
        {
            if (GetAccountID() == 0 || GetAccountInstance() != 0)
                return false;
        }

        if (GetAccountType() == utilpp::steam::EAccountType::GameServer)
        {
            if (GetAccountID() == 0)
                return false;
        }

        return true;
    }
    uint64_t GetValue()const {
        return Value.Bits.to_ullong();
    }
    void SetValue(uint64_t val) {
        Value.Bits.reset();
        Value.Bits |= val;
    }
    bool operator==(const FSteamID& rhs)const {
        return Value.Bits == rhs.Value.Bits;
    }
private:
    static constexpr uint8_t AccountIDBitOffset = 0;
    static constexpr uint8_t AccountIDBitLen = 32;
    static constexpr uint8_t AccountInstanceBitOffset = 32;
    static constexpr uint8_t AccountInstanceBitLen = 20;
    static constexpr uint8_t AccountTypeBitOffset = 52;
    static constexpr uint8_t AccountTypeBitLen = 4;
    static constexpr uint8_t AccountUniverseBitOffset = 56;
    static constexpr uint8_t AccountUniverseBitLen = 8;
    TBitVector<64> Value;
};