#pragma once

#include <type_traits>
#include <variant>
#include <stdexcept>
#include <string_view>


template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
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
To UnsafeVariantCast(From&& from)
{
    return std::visit([](auto&& elem) -> To
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

template <class _Type, template <class...> class _Template>
inline constexpr bool is_specialization_v = false; // true if and only if _Type is a specialization of _Template
template <template <class...> class _Template, class... _Types>
inline constexpr bool is_specialization_v<_Template<_Types...>, _Template> = true;

template <class _Type, template <class...> class _Template>
struct is_specialization : std::bool_constant<is_specialization_v<_Type, _Template>> {};


#if (defined(_MSVC_LANG) &&_MSVC_LANG <= 202002L && _MSVC_LANG >= 201112L) || (__cplusplus <= 202002L && __cplusplus >= 201112L)
#include <type_traits>
namespace std {
    template <class _Ty>
    constexpr underlying_type_t<_Ty> to_underlying(_Ty _Value) noexcept {
        return static_cast<underlying_type_t<_Ty>>(_Value);
    }
}
#endif

struct string_hash
{
    using hash_type = std::hash<std::string_view>;
    using hash_u8type = std::hash<std::u8string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const { return hash_type{}(str); }
    std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
    std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
    std::size_t operator()(const char8_t* str) const { return hash_u8type{}(str); }
    std::size_t operator()(std::u8string_view str) const { return hash_u8type{}(str); }
    std::size_t operator()(std::u8string str) const { return hash_u8type{}(str); }
};

struct pointer_hash
{
    std::size_t operator()(void* ptr) const { return std::size_t(ptr); }
};

struct hash_8bit
{
    std::size_t operator()(uint8_t key) const { return std::size_t(key); }
    std::size_t operator()(int8_t key) const { return std::size_t(key); }
};
struct hash_16bit
{
    std::size_t operator()(uint16_t key) const { return std::size_t(key); }
    std::size_t operator()(int16_t key) const { return std::size_t(key); }
};
struct hash_32bit
{
    std::size_t operator()(uint32_t key) const { return std::size_t(key); }
    std::size_t operator()(int32_t key) const { return std::size_t(key); }
};
struct hash_64bit
{
    std::size_t operator()(uint64_t key) const {
        if (sizeof(std::size_t) >= sizeof(uint64_t)) {
            return std::size_t(key);
        }
        else {
            return std::hash <uint64_t>{}(key);
        }
    }
    std::size_t operator()(int64_t key) const {
        if (sizeof(std::size_t) >= sizeof(int64_t)) {
            return std::size_t(key);
        }
        else {
            return std::hash <int64_t>{}(key);
        }
    }
};

template <typename Ret, typename... Args>
struct function_signature {
    using type = Ret(*)(Args...);
};

template <typename Ret, typename... Args>
using function_signature_t = typename function_signature<Ret, Args...>::type;