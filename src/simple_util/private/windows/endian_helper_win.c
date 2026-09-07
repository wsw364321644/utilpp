#include "endian_helper.h"
#include "simple_os_defs.h"

#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
uint64_t htobe64(uint64_t host_64bits) {
    return htonll(host_64bits);
}

uint64_t htobe32(uint32_t host_bits)
{
    return htonl(host_bits);
}

uint64_t htobe16(uint16_t host_bits)
{
    return htons(host_bits);
}
