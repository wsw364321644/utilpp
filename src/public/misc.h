/**
 *  misc.h
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <stdexcept>
#ifdef WIN32
#include <Windows.h>
#include <Strsafe.h>
#define F_HANDLE HANDLE
const uint32_t SK_CREATE_ALWAYS = CREATE_ALWAYS;
const uint32_t SK_OPEN_ALWAYS = OPEN_ALWAYS;
const uint32_t SK_OPEN_EXISTING = OPEN_EXISTING;
#else
#include <fcntl.h>
#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#include <cstring>
#define F_HANDLE int
const uint32_t SK_CREATE_ALWAYS = O_RDWR | O_CREAT | O_TRUNC;
const uint32_t SK_OPEN_ALWAYS = O_RDWR | O_CREAT;
const uint32_t SK_OPEN_EXISTING = O_RDWR;
#endif

#define ERR_SUCCESS			(0)		// success 
#define ERR_FAILED			(-1)	// common failure
#define ERR_ARGUMENT		(-2)	// argument error 
#define ERR_FILE			(-3)	// file operation related
#define ERR_METAPARSE		(-4)	// faile to parse mf file

namespace sonkwo
{
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }


    inline char Bin2Hex(char c)
    {
        if (c < 10) {
            return c += '0';
        }
        else {
            return c += 'A' - 10;
        }
    }


    template <typename T>
    struct Exactly {
        template <typename U, std::enable_if_t<std::is_same_v<T, U>, int> = 0>
        operator U() const;
    };

    template <typename To, typename From>
    To UnsafeVariantCast(From&& from)
    {
        return std::visit([](auto&& elem) -> To {
            using U = std::decay_t<decltype(elem)>;
            if constexpr (std::is_constructible_v<To, Exactly<U>>) {
                return To(std::forward<decltype(elem)>(elem));
            }
            else {
                throw std::runtime_error("Bad type");
            }
            }, std::forward<From>(from));
    }


    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };
}