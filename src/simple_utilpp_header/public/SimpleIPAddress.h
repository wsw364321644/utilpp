#pragma once
#include <variant>
#include <stdexcept>
#ifdef _WIN32
#include <simple_os_defs.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif


class FSimpleIPAddress {
public:
    using addr_variant = std::variant<sockaddr_in, sockaddr_in6>;
    addr_variant Addr;
    FSimpleIPAddress() = default;
    FSimpleIPAddress(const FSimpleIPAddress& inaddr) = default;

    explicit FSimpleIPAddress(const addrinfo* inaddr)
    {
        switch (inaddr->ai_family) {
        case AF_INET:
            this->Addr = reinterpret_cast<const sockaddr_in&>(*inaddr->ai_addr);
            break;
        case AF_INET6:
            this->Addr = reinterpret_cast<const sockaddr_in6&>(*inaddr->ai_addr);
            break;
        default:
            throw std::runtime_error("Unsupported network address");
        }
    }

    explicit FSimpleIPAddress(const sockaddr* inaddr)
    {
        operator =(inaddr);
    }

    FSimpleIPAddress(int family,std::string_view ipStr,short port)
    {

        switch (family) {
        case AF_INET: {
            sockaddr_in addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin_family = family;
            addr.sin_port = htons(port);
            if (inet_pton(family, ipStr.data(), &addr.sin_addr) != 1) {
                throw std::runtime_error("inet_pton error");
            }
            Addr = addr;
            break;
        }
        case AF_INET6: {
            sockaddr_in6 addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin6_family = family;
            addr.sin6_port = htons(port);
            if (inet_pton(family, ipStr.data(), &addr.sin6_addr) != 1) {
                throw std::runtime_error("inet_pton error");
            }
            Addr = addr;
            break;
        }
        default: {
            throw std::runtime_error("Unsupported network address");
        }
        }
    }

    bool operator==(const sockaddr* inaddr) {
        if (inaddr->sa_family != GetAdddr()->sa_family) {
            return false;
        }
        switch (inaddr->sa_family) {
        case AF_INET:
            return memcmp(&std::get<sockaddr_in>(Addr), inaddr, sizeof(sockaddr_in)) == 0;
            break;
        case AF_INET6:
            return memcmp(&std::get<sockaddr_in6>(Addr), inaddr, sizeof(sockaddr_in6)) == 0;
            break;
        default:
            return false;
        }
    }

    FSimpleIPAddress& operator=(const sockaddr* inaddr) {
        switch (inaddr->sa_family) {
        case AF_INET:
            this->Addr = reinterpret_cast<const sockaddr_in&>(*inaddr);
            break;
        case AF_INET6:
            this->Addr = reinterpret_cast<const sockaddr_in6&>(*inaddr);
            break;
        default:
            throw std::runtime_error("Unsupported network address");
        }
        return *this;
    }

    std::size_t AddrLen() const noexcept
    {
        return std::visit([]<class T>(const T & addr) noexcept -> std::size_t {
            return sizeof(T);
        }, Addr);
    }

    const sockaddr* GetAdddr() {
        return std::visit([]<class T>(const T & addr) noexcept -> const sockaddr* {
            return reinterpret_cast<const sockaddr*>(&addr);
        }, Addr);
    }

    int Connect(int socket) const
    {
#     if __cpp_generic_lambdas >= 201707L
        /* C++20: Use templated lambda */
        return std::visit([socket]<class T>(const T & addr) noexcept -> int {
            return ::connect(socket,
                reinterpret_cast<const sockaddr*>(&addr),
                sizeof(T));
        }, Addr);
#     else
        const sockaddr* ptr;
        int addrlen = 0;
        switch (Addr.index()) {
        case 0:
            ptr = reinterpret_cast<const sockaddr*>(&std::get<0>(Addr));
            addrlen = sizeof(sockaddr_in);
            break;
        case 1:
            ptr = reinterpret_cast<const sockaddr*>(&std::get<1>(Addr));
            addrlen = sizeof(sockaddr_in6);
            break;
        default:
            throw std::bad_variant_access{};
        }
        return ::connect(socket, ptr, addrlen);
#     endif
    }
};
