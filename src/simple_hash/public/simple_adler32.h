#pragma once
#include <stdint.h>
#include <assert.h>
#include <utility>
#include <vector>
#include "simple_hash_export_def.h"
class SIMPLE_HASH_EXPORT FRollingAdler32 {
public:
    static constexpr uint32_t MOD = 65521;
    static constexpr uint8_t CHAR_OFFSET = 0;
    FRollingAdler32() = delete;
    explicit FRollingAdler32(size_t window_size)
        : window_size_(window_size)
    {
        assert(window_size != 0);
        Reset();
    }
    /**
     * @brief 使用 SIMD 加速初始化（批量计算）
     * @param data 至少 window_size_ 字节的数据
     */
    void Init(const uint8_t* data);

    /**
     * @brief 滚动更新（标量实现，每次 1 字节）
     */
    void Roll(uint8_t out_byte, uint8_t new_byte);
    uint32_t Get() const {
        return (s2_ << 16) | s1_;
    }

    std::pair<uint32_t, uint32_t> GetComponent() const {
        return { s1_, s2_ };
    }

    void Reset() {
        s1_ = 1;
        s2_ = 0;
        full_ = false;
    }

    size_t WindowSize() const { return window_size_; }

private:

    size_t window_size_;
    uint32_t s1_;
    uint32_t s2_;
    bool full_;

};
