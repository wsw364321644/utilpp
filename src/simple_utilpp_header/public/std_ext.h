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
    std::size_t operator()(const std::string_view str) const { return hash_type{}(str); }
    std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
    std::size_t operator()(const char8_t* str) const { return hash_u8type{}(str); }
    std::size_t operator()(const std::u8string_view str) const { return hash_u8type{}(str); }
    std::size_t operator()(std::u8string const& str) const { return hash_u8type{}(str); }
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



template <class T>
struct allocator_save_memory_operator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using new_ptr_type = void* (*)(std::size_t);
    using delete_ptr_type = void (*)(void*);
    template <typename U>
    struct rebind {
        using other = allocator_save_memory_operator<U>;
    };

    constexpr allocator_save_memory_operator() noexcept :new_ptr(&::operator new), delete_ptr(&::operator delete) {

    }

    allocator_save_memory_operator(const allocator_save_memory_operator& other) noexcept {

        new_ptr = other.new_ptr;
        delete_ptr = other.delete_ptr;
    }

    allocator_save_memory_operator(allocator_save_memory_operator&& other) noexcept {
        new_ptr = other.new_ptr;
        delete_ptr = other.delete_ptr;
    }

    template <typename U>
    constexpr allocator_save_memory_operator(const allocator_save_memory_operator<U>& other) noexcept {
        new_ptr = other.new_ptr;
        delete_ptr = other.delete_ptr;
    }

    allocator_save_memory_operator select_on_container_copy_construction() const {
        return allocator_save_memory_operator(*this);
    }

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > max_size()) {
            throw std::bad_alloc{};
        }
        void* ptr = new_ptr(n * sizeof(T));
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        delete_ptr(p);
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
        //if constexpr (std::is_aggregate_v<U>) {
        //    // 对于聚合类型，使用花括号初始化
        //    ::new (static_cast<void*>(p)) U{ std::forward<Args>(args)... };
        //}
        //else {
        //    // 对于非聚合类型，使用圆括号初始化
        //    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
        //}
    }

    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

    [[nodiscard]] std::size_t max_size() const noexcept {
        return std::size_t(-1) / sizeof(T);
    }

    new_ptr_type new_ptr;
    delete_ptr_type delete_ptr;
};

template <typename T, typename U>
constexpr bool operator==(const allocator_save_memory_operator<T>& l, const allocator_save_memory_operator<U>& r) noexcept {
    return l.new_ptr == r.new_ptr;
}

template <typename T, typename U>
constexpr bool operator!=(const allocator_save_memory_operator<T>& l, const allocator_save_memory_operator<U>& r) noexcept {
    return l.new_ptr != r.new_ptr;
}
using string_save_memory_operator = std::basic_string<char, std::char_traits<char>, allocator_save_memory_operator<char>>;


#if (defined(_MSVC_LANG) &&_MSVC_LANG <= 202400L )
#include <time.h>
namespace std {
    inline struct tm* gmtime_r(const time_t* timer, struct tm* buf) {
        return gmtime_s(buf, timer) == 0 ? buf : nullptr;
    }
}
inline time_t mkgmtime(struct tm* const _Tm) {
#ifdef WIN32
    return _mkgmtime(_Tm);
#else
    return timegm(_Tm);
#endif // WIN32
}
//mkgmtime
#endif