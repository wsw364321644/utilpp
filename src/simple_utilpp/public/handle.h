#pragma once
#include <stdint.h>
#include <atomic>
#include <functional>
#include "simple_export_ppdefs.h"
struct NullCommonHandle_t {
    struct _Tag {};
    constexpr explicit NullCommonHandle_t(_Tag) {}
};
typedef struct SIMPLE_UTIL_EXPORT CommonHandle_t
{
    typedef  uint32_t CommonHandleID_t;
    constexpr CommonHandle_t(const NullCommonHandle_t):ID(0){ }
    CommonHandle_t() : CommonHandle_t(atomic_count) {}
    constexpr CommonHandle_t(CommonHandleID_t id) : ID(id) {}
    constexpr CommonHandle_t(const CommonHandle_t& handle) : ID(handle.ID) {}
    CommonHandle_t(std::atomic<CommonHandleID_t>&counter)
    {
        ID = ++counter ? counter.load() : ++counter;
    }
    virtual ~CommonHandle_t(){}
    bool IsValid() const
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

    bool operator==(const NullCommonHandle_t& handle) const
    {
        return !IsValid();
    }
    static std::atomic<CommonHandleID_t> atomic_count;
    CommonHandleID_t ID;
} CommonHandle_t;

constexpr CommonHandle_t::CommonHandleID_t NullCommonHandleID{ 0 };
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
