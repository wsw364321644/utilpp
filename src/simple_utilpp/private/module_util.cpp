#include "module_util.h"
#include "string_convert.h"
#include "char_buffer_extension.h"
#include "PathBuf.h"
#include <simple_os_defs.h>

EBinaryType get_binary_type(const char* path, int8_t* pointer_bytes) {
    FRawFile file;
    auto out = EBinaryType::FBT_NONE;
    auto ires=file.Open((const char8_t*)path, UTIL_OPEN_EXISTING);
    if (ires != ERR_SUCCESS) {
        return out;
    }
    unsigned char header[64];
    ires=file.Read(header, sizeof(header));
    if (ires != ERR_SUCCESS) {
        return out;
    }
    int8_t pointerBytes = 0;

    if (header[0] == 0x7F && header[1] == 'E' && header[2] == 'L' && header[3] == 'F') {
        out = EBinaryType::FBT_ELF;
        unsigned char elf_class = header[4];
        if (elf_class == 1) pointerBytes = sizeof(uint32_t);
        else if (elf_class == 2) pointerBytes =sizeof(uint64_t);
    }

    else if (header[0] == 'M' && header[1] == 'Z') {
        while (true) {
            unsigned int pe_offset = header[0x3C] | (header[0x3D] << 8) | (header[0x3E] << 16) | (header[0x3F] << 24);
            if (pe_offset > 0 && pe_offset < 1024) {
                file.Seek(pe_offset);
                unsigned char pe_header[26];
                ires = file.Read(pe_header, sizeof(pe_header));
                if (ires != ERR_SUCCESS) {
                    break;
                }
                if (pe_header[0] == 'P' && pe_header[1] == 'E' && pe_header[2] == 0 && pe_header[3] == 0) {
                    out = EBinaryType::FBT_PE;
                    unsigned short magic = pe_header[24] | (pe_header[25] << 8);
                    if (magic == 0x10B) {
                        pointerBytes = sizeof(uint32_t);
                    }
                    else if (magic == 0x20B) {
                        pointerBytes = sizeof(uint64_t);
                    }
                }
            }
            break;
        }
        if (out == EBinaryType::FBT_NONE) {
            out = EBinaryType::FBT_MZ;
            pointerBytes = sizeof(uint16_t);
        }
    }
    if (pointer_bytes) {
        *pointer_bytes = pointerBytes;
    }
    return out;
}