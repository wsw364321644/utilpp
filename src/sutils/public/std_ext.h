#pragma once

#include <type_traits>
#include <variant>
template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
struct Exactly
{
    template <typename U, std::enable_if_t<std::is_same_v<T, U>, int> = 0>
    operator U() const;
};

template <typename To, typename From>
To UnsafeVariantCast(From &&from)
{
    return std::visit([](auto &&elem) -> To
                      {
        using U = std::decay_t<decltype(elem)>;
        if constexpr (std::is_constructible_v<To, Exactly<U>>) {
            return To(std::forward<decltype(elem)>(elem));
        }
        else {
            throw std::runtime_error("Bad type");
        } },
                      std::forward<From>(from));
}

struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

#ifdef _MSVC_LANG
#if _MSVC_LANG <= 202002L and _MSVC_LANG >= 201112L
#include <type_traits>
namespace std {
    template <class _Ty>
    constexpr underlying_type_t<_Ty> to_underlying(_Ty _Value) noexcept {
        return static_cast<underlying_type_t<_Ty>>(_Value);
    }
}
#endif
#endif // _MSVC_LANG