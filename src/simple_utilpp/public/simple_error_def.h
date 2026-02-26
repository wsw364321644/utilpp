#pragma once
namespace utilpp {
    enum class ECommonUsedError :int32_t {
        CUE_OK = 0,
        CUE_NOT_SUPPORT = 0x1,
        CUE_UNKNOW=0x02,
        CUE_FILE_OP = 0x10,
        CUE_FILE_NOT_EXIST = 0x11,
    };
}