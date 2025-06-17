#pragma once
#include <stdint.h>
#include <atomic>
#include <functional>
#include <memory>
class ICommonHandle {
public:
    virtual ~ICommonHandle() = default;
    virtual bool IsValid() const = 0;
};
typedef std::shared_ptr<ICommonHandle> FCommonHandlePtr;

struct NullCommonHandle_t {
    struct _Tag {};
    constexpr explicit NullCommonHandle_t(_Tag) {}
};
typedef struct CommonHandle_t:public ICommonHandle
{
    typedef  uint32_t CommonHandleID_t;
    constexpr CommonHandle_t() :ID(0) {}
    constexpr CommonHandle_t(const NullCommonHandle_t):CommonHandle_t() { }
    constexpr CommonHandle_t(CommonHandleID_t id) : ID(id) {}
    constexpr CommonHandle_t(const CommonHandle_t& handle) : ID(handle.ID) {}
    CommonHandle_t(std::atomic<CommonHandleID_t>&counter)
    {
        ID = counter.fetch_add(1,std::memory_order::relaxed);
        if (ID==0) {
            ID = counter.fetch_add(1, std::memory_order::relaxed);
        }
    }
    virtual ~CommonHandle_t(){}
    bool IsValid() const override
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
    inline static std::atomic<CommonHandleID_t> atomic_count{0};
    CommonHandleID_t ID;
} CommonHandle_t;

constexpr CommonHandle_t::CommonHandleID_t NullCommonHandleID{ 0 };
inline constexpr NullCommonHandle_t NullHandle{ NullCommonHandle_t::_Tag{} };

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
