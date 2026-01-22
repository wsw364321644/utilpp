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
inline constexpr NullCommonHandle_t NullHandle{ NullCommonHandle_t::_Tag{} };



template<typename T>
struct CommonHandle_t:public ICommonHandle
{
    typedef  T CommonHandleID_t;
    static constexpr CommonHandleID_t NullCommonHandleID{ 0 };

    constexpr CommonHandle_t() :ID(NullCommonHandleID) {}
    constexpr CommonHandle_t(const NullCommonHandle_t):CommonHandle_t() { }
    constexpr CommonHandle_t(CommonHandleID_t id) : ID(id) {}
    constexpr CommonHandle_t(const CommonHandle_t& handle) : ID(handle.ID) {}
    CommonHandle_t(std::atomic<CommonHandleID_t>&counter)
    {
        ID = counter.fetch_add(1,std::memory_order::relaxed);
        if (!IsValid()) {
            ID = counter.fetch_add(1, std::memory_order::relaxed);
        }
    }
    virtual ~CommonHandle_t(){}
    bool IsValid() const override
    {
        return ID != NullCommonHandleID;
    }
    void Reset()
    {
        ID = NullCommonHandleID;
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
    auto& operator=(const NullCommonHandle_t& handle)
    {
        Reset();
        return *this;
    }
    operator bool() const
    {
        return IsValid();
    }
    inline static std::atomic<CommonHandleID_t> atomic_count{ NullCommonHandleID };
    CommonHandleID_t ID;
};

typedef CommonHandle_t<uint32_t> CommonHandle32_t;
typedef CommonHandle_t<intptr_t> CommonHandlePtr_t;

namespace std
{
    template <typename T>
    struct equal_to<CommonHandle_t<T>>
    {
        using argument_type = CommonHandle_t<T>;
        using result_type = bool;
        constexpr bool operator()(const argument_type&lhs, const argument_type&rhs) const
        {
            return lhs.ID == rhs.ID;
        }
    };

    template <typename T>
    class hash<CommonHandle_t<T>>
    {
    public:
        size_t operator()(const CommonHandle_t<T>&handle) const
        {
            return handle.ID;
        }
    };

    template <typename T>
    struct less<CommonHandle_t<T>>
    {
    public:
        size_t operator()(const CommonHandle_t<T>&_Left, const CommonHandle_t<T>&_Right) const
        {
            return _Left.operator<(_Right);
        }
    };
}
