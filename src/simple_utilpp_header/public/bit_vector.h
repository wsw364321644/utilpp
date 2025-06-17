#pragma once
#include <bitset>
#include <unordered_map>
#include "std_ext.h"

template <size_t _Bits>
class TBitVector {
public:
    TBitVector() = default;
    constexpr bool operator[](size_t _Pos) const noexcept {
        Bits[_Pos];
    }
    template<typename T>
    void SetBitSet(T bitset, size_t offset, size_t bit_num) {
        std::bitset<_Bits> target(bitset);
        ClearBit(offset, bit_num);
        Bits |= target << offset;
    }
    template<typename T>
    T GetBitSet(size_t offset, size_t bit_num) const {
        std::bitset<_Bits> mask;
        mask.flip();
        mask >>= _Bits - bit_num;
        mask <<= offset;
        return T((Bits & mask >> offset).to_ullong());
    }
    std::bitset<_Bits> Bits;
private:
    inline void ClearBit(size_t& offset, size_t& bit_num) {
        std::bitset<_Bits> mask;
        mask.flip();
        mask >>= _Bits - bit_num;
        mask <<= offset;
        mask.flip();
        Bits &= mask;
    }
    
};