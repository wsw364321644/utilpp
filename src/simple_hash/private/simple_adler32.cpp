#include "simple_adler32.h"
#include <zlib.h>

void FRollingAdler32::Init(const uint8_t* data)
{
    uint32_t s1, s2;
    s1=adler32(s1_, data, window_size_);

    s2_ = s1>>16;
    s1_ = s1% MOD;
    full_ = true;
}

void FRollingAdler32::Roll(uint8_t out_byte,uint8_t new_byte)
{
    uint8_t c = new_byte + CHAR_OFFSET;
    s1_ += c;
    if (s1_ >= MOD) s1_ -= MOD;
    s2_ += s1_;
    if (s2_ >= MOD) s2_ -= MOD;

    // 移除旧字节
    uint32_t tmp = s1_ + MOD - (out_byte + CHAR_OFFSET);
    s1_ = (tmp >= MOD) ? (tmp - MOD) : tmp;

    tmp = s2_ + MOD - (window_size_ * (out_byte + CHAR_OFFSET) % MOD);
    s2_ = (tmp >= MOD) ? (tmp - MOD) : tmp;

    // 添加新字节
    s1_ += new_byte + CHAR_OFFSET;
    if (s1_ >= MOD) s1_ -= MOD;

    s2_ += s1_;
    if (s2_ >= MOD) s2_ -= MOD;
}
