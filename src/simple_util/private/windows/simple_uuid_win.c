#include "simple_uuid.h"
#include "simple_os_defs.h"
#include <rpc.h>
#include <rpcdce.h>
#pragma comment(lib, "Rpcrt4.lib")
void generate_uuid_128(uint8_t* const des) {
    UuidCreate((UUID*)des);
}