#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <stdexcept>

template<size_t kBits>
class TBitVector {
    static_assert(kBits > 0, "kBits must be greater than 0");

public:
    /// @brief 构造函数
    constexpr TBitVector() noexcept {
        ClearAll();
    }

    /// @brief 从整数初始化
    explicit constexpr TBitVector(uint64_t value) noexcept {
        ClearAll();
        if (kBits <= 64) {
            for (size_t i = 0; i < std::min(kBits, (size_t)64); ++i) {
                if (value & (1ULL << i)) {
                    Set(i);
                }
            }
        }
    }

    // ==================== 元素访问 ====================

    /// @brief 访问指定位置的位
    constexpr bool operator[](size_t pos) const noexcept {
        return Test(pos);
    }

    /// @brief 测试指定位置的位
    constexpr bool Test(size_t pos) const noexcept {
        return (data_[pos / 8] >> (pos % 8)) & 1U;
    }

    // ==================== 设置操作 ====================

    /// @brief 设置指定位置的位为 1
    constexpr void Set(size_t pos) noexcept {
        data_[pos / 8] |= (1U << (pos % 8));
    }

    /// @brief 清除指定位置的位为 0
    constexpr void Reset(size_t pos) noexcept {
        data_[pos / 8] &= ~(1U << (pos % 8));
    }

    /// @brief 翻转指定位置的位
    constexpr void Flip(size_t pos) noexcept {
        data_[pos / 8] ^= (1U << (pos % 8));
    }

    /// @brief 设置所有位
    constexpr void SetAll() noexcept {
        memset(data_, 0xFF, sizeof(data_));
        // 清除超出 kBits 的位
        if (kBits % 8 != 0) {
            data_[kWords - 1] &= ((1U << (kBits % 8)) - 1U);
        }
    }

    /// @brief 清除所有位
    constexpr void ClearAll() noexcept {
        memset(data_, 0, sizeof(data_));
    }

    // ==================== 批量操作 ====================

    /// @brief 设置指定范围的位
    /// @tparam T 值类型
    /// @param value 要设置的值
    /// @param offset 起始位置
    /// @param bit_num 位数
    template<typename T>
    void SetBits(T value, size_t offset, size_t bit_num) {
        if (bit_num == 0) return;
        assert(offset + bit_num <= kBits);

        // 先清除目标区域
        ClearBits(offset, bit_num);

        // 逐位设置
        for (size_t i = 0; i < bit_num; ++i) {
            if (value & (T(1) << i)) {
                size_t pos = offset + i;
                data_[pos / 8] |= (1U << (pos % 8));
            }
        }
    }

    /// @brief 获取指定范围的位
    template<typename T>
    T GetBits(size_t offset, size_t bit_num) const {
        if (bit_num == 0) return T(0);
        assert(offset + bit_num <= kBits);

        T result = 0;
        for (size_t i = 0; i < bit_num; ++i) {
            size_t pos = offset + i;
            if ((data_[pos / 8] >> (pos % 8)) & 1U) {
                result |= (T(1) << i);
            }
        }
        return result;
    }

    /// @brief 清除指定范围的位
    void ClearBits(size_t offset, size_t bit_num) {
        if (bit_num == 0) return;
        assert(offset + bit_num <= kBits);

        for (size_t i = 0; i < bit_num; ++i) {
            size_t pos = offset + i;
            data_[pos / 8] &= ~(1U << (pos % 8));
        }
    }

    // ==================== 查询操作 ====================

    /// @brief 获取位数
    constexpr size_t Size() const noexcept {
        return kBits;
    }

    /// @brief 获取1的数量
    constexpr size_t Count() const noexcept {
        size_t count = 0;
        for (size_t i = 0; i < kWords; ++i) {
            count += __builtin_popcount(data_[i]);
        }
        // 清除超出部分的计数
        if (kBits % 8 != 0) {
            uint8_t last_mask = (1U << (kBits % 8)) - 1U;
            count -= __builtin_popcount(data_[kWords - 1] & ~last_mask);
        }
        return count;
    }

    /// @brief 检查是否全为0
    constexpr bool None() const noexcept {
        for (size_t i = 0; i < kWords; ++i) {
            if (data_[i] != 0) return false;
        }
        return true;
    }

    /// @brief 检查是否全为1
    constexpr bool All() const noexcept {
        for (size_t i = 0; i < kWords - 1; ++i) {
            if (data_[i] != UINT8_MAX) return false;
        }
        // 检查最后一个byte
        uint8_t last_mask = (kBits % 8 == 0) ? UINT8_MAX : ((1U << (kBits % 8)) - 1U);
        return (data_[kWords - 1] & last_mask) == last_mask;
    }

    /// @brief 检查是否有任何1
    constexpr bool Any() const noexcept {
        return !None();
    }

    // ==================== 查找操作 ====================

    /// @brief 查找第一个1的位置
    constexpr size_t FindFirst() const noexcept {
        for (size_t i = 0; i < kWords; ++i) {
            if (data_[i] != 0) {
                return i * 8 + __builtin_ctz(data_[i]);
            }
        }
        return kBits;
    }

    /// @brief 查找第一个0的位置
    constexpr size_t FindFirstZero() const noexcept {
        uint8_t last_mask = (kBits % 8 == 0) ? UINT8_MAX : ((1U << (kBits % 8)) - 1U);

        for (size_t i = 0; i < kWords - 1; ++i) {
            if (data_[i] != UINT8_MAX) {
                return i * 8 + __builtin_ctz(~data_[i]);
            }
        }
        // 检查最后一个byte
        if ((data_[kWords - 1] & last_mask) != last_mask) {
            return (kWords - 1) * 8 + __builtin_ctz(~data_[kWords - 1]);
        }
        return kBits;
    }

    /// @brief 查找下一个1的位置
    constexpr size_t FindNext(size_t pos) const noexcept {
        if (pos >= kBits) return kBits;

        size_t byte_idx = pos / 8;
        size_t bit_pos = pos % 8;

        // 检查当前byte
        uint8_t byte = data_[byte_idx] >> bit_pos;
        if (byte != 0) {
            return pos + __builtin_ctz(byte);
        }

        // 检查后续bytes
        for (size_t i = byte_idx + 1; i < kWords; ++i) {
            if (data_[i] != 0) {
                return i * 8 + __builtin_ctz(data_[i]);
            }
        }
        return kBits;
    }

    // ==================== 批量操作 ====================

    /// @brief 批量设置
    void SetBatch(size_t start, size_t count) {
        size_t end = std::min(start + count, kBits);
        size_t idx = start;

        // 处理不对齐的前缀
        while (idx < end && (idx % 8)) {
            Set(idx++);
        }

        // 对齐后批量处理（每次处理8字节）
        while (idx + 8 <= end) {
            data_[idx / 8] = UINT8_MAX;
            idx += 8;
        }

        // 处理尾部
        while (idx < end) {
            Set(idx++);
        }
    }

    /// @brief 批量清除
    void ClearBatch(size_t start, size_t count) {
        size_t end = std::min(start + count, kBits);
        size_t idx = start;

        while (idx < end && (idx % 8)) {
            Reset(idx++);
        }

        while (idx + 8 <= end) {
            data_[idx / 8] = 0;
            idx += 8;
        }

        while (idx < end) {
            Reset(idx++);
        }
    }

    // ==================== 数据访问 ====================

    /// @brief 获取原始数据指针（只读）
    const uint8_t* Data() const noexcept {
        return data_;
    }

    /// @brief 获取原始数据指针
    uint8_t* Data() noexcept {
        return data_;
    }

    /// @brief 获取字节数
    constexpr size_t DataSize() const noexcept {
        return kWords;
    }

    /// @brief 导出到 std::bitset
    template<size_t N>
    std::bitset<N> ToBitset() const {
        static_assert(N >= kBits, "N must be >= kBits");
        std::bitset<N> result;
        for (size_t i = 0; i < kWords; ++i) {
            uint8_t byte = data_[i];
            for (size_t j = 0; j < 8 && i * 8 + j < kBits; ++j) {
                if (byte & (1U << j)) {
                    result.set(i * 8 + j);
                }
            }
        }
        return result;
    }

    // ==================== 运算符重载 ====================

    TBitVector operator&(const TBitVector& other) const noexcept {
        TBitVector result;
        for (size_t i = 0; i < kWords; ++i) {
            result.data_[i] = data_[i] & other.data_[i];
        }
        return result;
    }

    TBitVector operator|(const TBitVector& other) const noexcept {
        TBitVector result;
        for (size_t i = 0; i < kWords; ++i) {
            result.data_[i] = data_[i] | other.data_[i];
        }
        return result;
    }

    TBitVector operator^(const TBitVector& other) const noexcept {
        TBitVector result;
        for (size_t i = 0; i < kWords; ++i) {
            result.data_[i] = data_[i] ^ other.data_[i];
        }
        return result;
    }

    TBitVector operator~() const noexcept {
        TBitVector result;
        for (size_t i = 0; i < kWords; ++i) {
            result.data_[i] = ~data_[i];
        }
        // 清除超出位
        if (kBits % 8 != 0) {
            result.data_[kWords - 1] &= ((1U << (kBits % 8)) - 1U);
        }
        return result;
    }

    // ==================== 比较 ====================

    constexpr bool operator==(const TBitVector& other) const noexcept {
        for (size_t i = 0; i < kWords; ++i) {
            if (data_[i] != other.data_[i]) return false;
        }
        return true;
    }

    constexpr bool operator!=(const TBitVector& other) const noexcept {
        return !(*this == other);
    }

private:
    static constexpr size_t kWords = (kBits + 7) / 8;
    uint8_t data_[kWords];
};
