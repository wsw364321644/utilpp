#include "endian_helper.h"
#include "simple_os_defs.h"

#include <winsock2.h>
uint64_t htobe64(uint64_t host_64bits) {
    return htonll(host_64bits);
}