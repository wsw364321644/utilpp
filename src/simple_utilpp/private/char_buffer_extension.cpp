#include "char_buffer_extension.h"
#include "simple_uuid.h"
#include "hex.h"

bool LoadFileToCharBuffer(FRawFile& file, FCharBuffer& buf, size_t extraSpace)
{
    if (file.IsOpen() == false) {
        return false;
    }
    auto size=file.GetSize();
    buf.Reserve(size + extraSpace);
    if (file.Read(buf.Data(), size) != ERR_SUCCESS) {
        return false;
    }
    buf.SetLength(size);
    return true;
}

bool GenHexUUID(FCharBuffer& charbuf)
{
    uint8_t buf[UUID_128_BYTES];
    generate_uuid_128(buf);
    charbuf.Reserve(bin_to_hex_length(UUID_128_BYTES) + 1);
    charbuf.SetLength(bin_to_hex_length(UUID_128_BYTES));
    return to_upper_hex(charbuf.Data(), buf, UUID_128_BYTES);
}
