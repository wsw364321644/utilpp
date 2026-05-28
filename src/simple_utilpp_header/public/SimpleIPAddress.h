#pragma once
#include <variant>
#include <stdexcept>
#include <system_error>
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
    sockaddr_storage Addr;
    FSimpleIPAddress() = default;
    FSimpleIPAddress(const FSimpleIPAddress& inaddr) = default;

    explicit FSimpleIPAddress(const addrinfo* inaddr)
    {
        operator =(inaddr->ai_addr);
    }

    explicit FSimpleIPAddress(const sockaddr* inaddr)
    {
        operator =(inaddr);
    }

    FSimpleIPAddress(int family,std::string_view ipStr,short port)
    {

        switch (family) {
        case AF_INET: {
            sockaddr_in& addr= *reinterpret_cast<sockaddr_in *>(&Addr);
            std::memset(&Addr, 0, sizeof(addr));
            addr.sin_family = family;
            addr.sin_port = htons(port);
            auto ires = inet_pton(family, ipStr.data(), &addr.sin_addr);

            if(ires == 0) {
                throw std::invalid_argument("invalid address");
            }
            else if (ires == -1) {
                throw std::error_code(WSAGetLastError(), std::system_category());
            }
            break;
        }
        case AF_INET6: {
            sockaddr_in6& addr = *reinterpret_cast<sockaddr_in6*>(&Addr);
            std::memset(&Addr, 0, sizeof(addr));
            addr.sin6_family = family;
            addr.sin6_port = htons(port);
            auto ires = inet_pton(family, ipStr.data(), &addr.sin6_addr);
            if (ires == 0) {
                throw std::invalid_argument("invalid address");
            }
            else if (ires == -1) {
                throw std::error_code(WSAGetLastError(), std::system_category());
            }
            break;
        }
        default: {
            throw std::invalid_argument("Unsupported network address");
        }
        }
    }

    bool operator==(const sockaddr* inaddr) {
        if (inaddr->sa_family != GetAdddr()->sa_family) {
            return false;
        }
        switch (inaddr->sa_family) {
        case AF_INET:
            return memcmp(&Addr, inaddr, sizeof(sockaddr_in)) == 0;
        case AF_INET6:
            return memcmp(&Addr, inaddr, sizeof(sockaddr_in6)) == 0;
        default:
            return false;
        }
    }

    FSimpleIPAddress& operator=(const sockaddr* inaddr) {
        switch (inaddr->sa_family) {
        case AF_INET:
            memcpy(&Addr, inaddr, sizeof(sockaddr_in));
            break;
        case AF_INET6:
            memcpy(&Addr, inaddr, sizeof(sockaddr_in6));
            break;
        default:
            throw std::invalid_argument("Unsupported network address");
        }
        return *this;
    }

    std::size_t AddrLen() const noexcept
    {
        switch (Addr.ss_family) {
        case AF_INET:
            return sizeof(sockaddr_in);
        case AF_INET6:
            return sizeof(sockaddr_in6);
        }
    }

    const sockaddr* GetAdddr() {
        return reinterpret_cast<const sockaddr*>(&Addr);
    }

    int Connect(int socket) const
    {
        return ::connect(socket,reinterpret_cast<const sockaddr*>(&Addr),AddrLen());
    }
};
