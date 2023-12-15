#pragma once
#include <stdint.h>
#include <atomic>
#include <functional>


typedef struct CommonHandle
{
    struct _Tag {};
    constexpr explicit CommonHandle(_Tag):ID(0){ }
    CommonHandle()
    {
        ID = 0;
    }
    CommonHandle(uint32_t id)
    {
        ID = id;
    }
    CommonHandle(const CommonHandle &handle)
    {
        ID = handle.ID;
    }
    CommonHandle(std::atomic_uint32_t &counter)
    {
        ID = ++counter ? counter.load() : ++counter;
    }
    bool IsValid()
    {
        return ID != 0;
    }

    bool operator<(const CommonHandle &handle) const
    {
        return ID < handle.ID;
    }
    bool operator==(const CommonHandle &handle) const
    {
        return ID == handle.ID;
    }

    static std::atomic_uint32_t atomic_count;
    uint32_t ID;
} CommonHandle_t;
inline constexpr CommonHandle NullHandle{ CommonHandle::_Tag{} };
// inline bool operator< (const CommonHandle_t& lhs, const CommonHandle_t& rhs) {
//     return lhs < rhs;
// }
// inline bool operator== (const CommonHandle_t& lhs, const CommonHandle_t& rhs) {
//     return lhs == rhs;
// }
namespace std
{
    template <>
    struct equal_to<CommonHandle_t>
    {
        using argument_type = CommonHandle_t;
        using result_type = bool;
        constexpr bool operator()(const CommonHandle_t &lhs, const CommonHandle_t &rhs) const
        {
            return lhs.ID == rhs.ID;
        }
    };

    template <>
    class hash<CommonHandle_t>
    {
    public:
        size_t operator()(const CommonHandle_t &handle) const
        {
            return handle.ID;
        }
    };

    template <>
    struct less<CommonHandle_t>
    {
    public:
        size_t operator()(const CommonHandle_t &_Left, const CommonHandle_t &_Right) const
        {
            return _Left.operator<(_Right);
        }
    };
}
