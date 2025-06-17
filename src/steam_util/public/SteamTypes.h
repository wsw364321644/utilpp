#pragma once
#include <stdint.h>
#include <chrono>
#include <type_traits>
#include <bit_vector.h>
constexpr auto SteamStartTimeEpoch = std::chrono::sys_days(std::chrono::year_month_day(std::chrono::January / 1 / 2005));
class FSteamGlobalID {
public:
    FSteamGlobalID() = default;
    FSteamGlobalID(uint64_t val) {
        SetValue(val);
    }
    FSteamGlobalID(const FSteamGlobalID& inID) {
        Value = inID.Value;
    }
    uint32_t GetSequentialCount() const {
        return Value.template GetBitSet<uint32_t>(SequentialCountBitOffset, SequentialCountBitLen);
    }
    void SetSequentialCount(uint32_t val) {
        Value.SetBitSet(val, SequentialCountBitOffset, SequentialCountBitLen);
    }
    std::chrono::time_point<std::chrono::system_clock> GetStartTime() const {
        auto sec = Value.template GetBitSet<uint64_t>(StartTimeBitOffset, StartTimeBitLen);
        std::chrono::seconds duration_seconds(sec);
        return SteamStartTimeEpoch + duration_seconds;
    }
    void SetStartTime(std::chrono::time_point<std::chrono::system_clock> time) {
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(time - SteamStartTimeEpoch).count();
        Value.SetBitSet(sec, StartTimeBitOffset, StartTimeBitLen);
    }
    uint32_t GetProcessID() const {
        return Value.template GetBitSet<uint32_t>(ProcessIDBitOffset, ProcessIDBitLen);
    }
    void SetProcessID(uint32_t val) {
        Value.SetBitSet(val, ProcessIDBitOffset, ProcessIDBitLen);
    }
    uint32_t GetBoxID() const {
        return Value.template GetBitSet<uint32_t>(BoxIDBitOffset, BoxIDBitLen);
    }
    void SetBoxID(uint32_t val) {
        Value.SetBitSet(val, BoxIDBitOffset, BoxIDBitLen);
    }
    uint64_t GetValue()const {
        return Value.Bits.to_ullong();
    }
    void SetValue(uint64_t val) {
        Value.Bits.reset();
        Value.Bits |= val;
    }
    bool operator==(const FSteamGlobalID& rhs)const {
        return Value.Bits==rhs.Value.Bits;
    }
private:
    static constexpr uint8_t SequentialCountBitOffset = 0;
    static constexpr uint8_t SequentialCountBitLen = 20;
    static constexpr uint8_t StartTimeBitOffset = 20;
    static constexpr uint8_t StartTimeBitLen = 30;
    static constexpr uint8_t ProcessIDBitOffset = 50;
    static constexpr uint8_t ProcessIDBitLen = 4;
    static constexpr uint8_t BoxIDBitOffset = 54;
    static constexpr uint8_t BoxIDBitLen = 10;
    TBitVector<64> Value;
};


namespace std {
    template<>
    struct hash<FSteamGlobalID> {
        size_t operator()(const FSteamGlobalID& _Keyval) const noexcept {
            return hash<uint64_t>()(_Keyval.GetValue());
        }
    };
}
