#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>
#include <variant>
#include <string>
#include <string_buffer.h>
#include "delegate_macros.h"
#pragma warning(push)
#pragma warning(disable:4251)

#define REGISTER_RPC_API(APIName,ClassName,RequestRecvFunc,ResponseRecvFunc) \
    const char ClassName::APIName##Name[] = #APIName; \
    static RPCInfoRegister<##ClassName> APIName##Register(RPCMethodInfo<##ClassName>{.Name = ClassName::APIName##Name, \
        .OnRequestMethod = &##ClassName::##RequestRecvFunc, \
        .OnResponseMethod = &##ClassName::##ResponseRecvFunc \
    });
#define REGISTER_RPC_EVENT_API(APIName,ClassName,RequestRecvFunc) \
    const char ClassName::APIName##Name[] = #APIName; \
    static RPCInfoRegister<##ClassName> APIName##Register(RPCMethodInfo<##ClassName>{.Name = ClassName::APIName##Name, \
        .OnRequestMethod = &##ClassName::##RequestRecvFunc, \
        .OnResponseMethod = nullptr \
    });
enum class ERPCParseError {
    OK,
    ParseError,
    InternalError
};


class RPCRequest {
public:
    virtual ~RPCRequest() = default;
    virtual CharBuffer ToBytes() = 0;

    std::optional<uint32_t> ID;
    std::string Method;
    std::string Params;
};
class RPCResponse {
public:
    virtual ~RPCResponse() = default;
    virtual CharBuffer ToBytes() = 0;
    uint32_t ID;
    double ErrorCode;
    std::string ErrorMsg;
    std::string ErrorData;
    std::string Result;
};
class IRPCPaser {
public:
    typedef std::variant<ERPCParseError, std::shared_ptr<RPCRequest>, std::shared_ptr<RPCResponse>> ParseResult;
    virtual ParseResult Parse(const char* data, int len) = 0;
};

#pragma warning(pop)