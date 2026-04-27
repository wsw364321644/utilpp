#pragma once
namespace utilpp {
    enum class ECommonUsedError :int32_t {
        CUE_OK = 0,
        CUE_NOT_SUPPORT = 0x1,
        CUE_UNKNOW=0x02,
        CUE_REQ_TOO_MANY = 0x03,
        CUE_FILE_OP = 0x10,
        CUE_FILE_NOT_EXIST = 0x11,
        CUE_NET_ERROR = 0x80,
        CUE_WAIT_TWO_FACTOR = 0x81,
        CUE_NEED_LOGIN = 0x82,
    };
}