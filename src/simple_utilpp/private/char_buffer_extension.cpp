#include "char_buffer_extension.h"


bool LoadFileToCharBuffer(FRawFile& file, FCharBuffer& buf, size_t extraSpace)
{
    if (file.IsOpen() == false) {
        return false;
    }
    auto size=file.GetSize();
    buf.Reverse(size + extraSpace);
    if (file.Read(buf.Data(), size) != ERR_SUCCESS) {
        return false;
    }
    buf.SetLength(size);
    return true;
}
