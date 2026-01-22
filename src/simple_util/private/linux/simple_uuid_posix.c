#include "simple_uuid.h"
#include "simple_os_defs.h"
#include <uuid/uuid.h>
void generate_uuid_128(uint8_t* const des) {
    uuid_t uuid;
    uuid_generate(uuid);
    memcpy(des, uuid, sizeof(uuid_t));
}