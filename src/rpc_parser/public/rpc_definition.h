#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <optional>
#include <variant>
#include <string>
#include <string_buffer.h>
#include "delegate_macros.h"
#include "rpc_parser_common.h"
#pragma warning(push)
#pragma warning(disable:4251)

enum class ERPCParseError {
    OK,
    ParseError,
    InvalidRequest,
    InternalError
};


class RPC_PARSER_EXPORT RPCRequest {
public:
    virtual ~RPCRequest() = default;
    virtual CharBuffer ToBytes() = 0;

    std::optional<uint32_t> ID;
    std::string Method;
    std::string Params;
};
class RPC_PARSER_EXPORT RPCResponse {
public:
    virtual ~RPCResponse() = default;
    virtual CharBuffer ToBytes() = 0;
    std::optional<uint32_t> ID;
    double ErrorCode{ 0 };
    std::string ErrorMsg;
    std::string ErrorData;
    std::string Result;
};
class IRPCPaser {
public:
    typedef std::variant<ERPCParseError, std::shared_ptr<RPCRequest>, std::shared_ptr<RPCResponse>> ParseResult;
    virtual ParseResult Parse(const char* data, int len) = 0;
    virtual std::shared_ptr<RPCResponse> GetMethodNotFoundResponse(std::optional<uint32_t> id) = 0;
    virtual std::shared_ptr<RPCResponse> GetErrorParseResponse(ERPCParseError error) = 0;
};

#pragma warning(pop)