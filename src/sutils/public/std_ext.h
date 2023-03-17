#pragma once

#include <type_traits>

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