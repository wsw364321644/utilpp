#pragma once
#include <stdint.h>
#include <atomic>
#include <functional>

struct NullCommonHandle_t {
    struct _Tag {};
    constexpr explicit NullCommonHandle_t(_Tag) {}
};
typedef struct CommonHandle_t
{
    constexpr explicit CommonHandle_t(NullCommonHandle_t):ID(0){ }
    constexpr CommonHandle_t() : ID(0) {}
    constexpr CommonHandle_t(uint32_t id) : ID(id) {}
    constexpr CommonHandle_t(const CommonHandle_t& handle) : ID(handle.ID) {}
    CommonHandle_t(std::atomic_uint32_t &counter)
    {
        ID = ++counter ? counter.load() : ++counter;
    }
    virtual ~CommonHandle_t(){}
    bool IsValid()
    {
        return ID != 0;
    }

    bool operator<(const CommonHandle_t&handle) const
    {
        return ID < handle.ID;
    }
    bool operator==(const CommonHandle_t&handle) const
    {
        return ID == handle.ID;
    }

    static std::atomic_uint32_t atomic_count;
    uint32_t ID;
} CommonHandle_t;

inline constexpr NullCommonHandle_t NullHandle{ NullCommonHandle_t::_Tag{} };
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
