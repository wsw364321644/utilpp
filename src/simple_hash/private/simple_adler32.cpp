#include "simple_adler32.h"
#include <zlib.h>

FRollingAdler32::FRollingAdler32()
{
    Reset();
}

void FRollingAdler32::Init(const uint8_t* data, uint32_t window_size)
{
    assert(window_size != 0);
    uint32_t s1, s2;
    s1=adler32(s1_, data, window_size);

    window_size_ = window_size;
    s2_ = s1>>16;
    s1_ = s1% MOD;
    full_ = true;
}

void FRollingAdler32::Roll(uint8_t out_byte,uint8_t new_byte)
{
    // 移除旧字节 out_byte 对 s1 的影响
    uint32_t tmp_s1 = s1_ + MOD - out_byte; // 加 MOD 保证非负
    s1_ = (tmp_s1 >= MOD) ? (tmp_s1 - MOD) : tmp_s1;

    // 移除旧字节 out_byte 对 s2 的影响
    // s2 是所有 (字符 * 其位置索引) 的累积和。
    // 移除最老字符时，相当于减去 (out_byte * window_size_)，因为该字符在窗口中的位置索引最大（假设为 window_size_-1 到 0），
    // 但这等价于从 s2 中减去 (s1_before_out + s1_after_out_shifted)，简化后就是 s2_ -= (window_size_ * out_byte)。
    // 由于直接计算可能涉及较大数值，我们用模运算性质：
    // s2_ = s2_ - (window_size_ * out_byte) + k * MOD (为了保证非负)
    // 即 s2_ = (s2_ - (window_size_ * out_byte % MOD) + MOD) % MOD
    // 等价于下面的实现：
    uint32_t contribution_of_out_byte_to_s2 = (static_cast<uint32_t>(window_size_) * out_byte) % MOD;
    uint32_t tmp_s2 = s2_ + MOD - contribution_of_out_byte_to_s2; // 加 MOD 保证非负
    s2_ = (tmp_s2 >= MOD) ? (tmp_s2 - MOD) : tmp_s2;

    // 添加新字节 new_byte 对 s1 的影响
    s1_ += new_byte;
    if (s1_ >= MOD) s1_ -= MOD;

    // 添加新字节 new_byte 对 s2 的影响 (它现在处于窗口的最后一位，即索引0)
    s2_ += s1_; // s1_ 已经包含了 new_byte
    if (s2_ >= MOD) s2_ -= MOD;
}
