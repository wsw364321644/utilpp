#pragma once
namespace utilpp {
    enum class EIPCError :int {
        IPCE_OK = 0,
        IPCE_AlreadyExist,
        IPCE_NotExist,
        IPCE_Timeout,
        IPCE_Unknow,
    };
}