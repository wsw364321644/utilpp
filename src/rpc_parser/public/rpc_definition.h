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
    friend class JRPCPaser;
public:
    virtual ~RPCRequest() = default;
    virtual CharBuffer ToBytes() = 0;
    void SetParams(const std::string& str) {
        Params = str;
    }
    void SetParams(const char* cstr) {
        Params.assign(cstr);
    }
    std::string_view GetParams()const {
        return Params;
    }

    void SetMethod(const std::string& str) {
        Method = str;
    }
    void SetMethod(const char* cstr) {
        Method.assign(cstr);
    }
    std::string_view GetMethod()const {
        return Method;
    }

    bool HasID() const{
        return ID.has_value();
    }
    uint32_t GetID() const {
        return ID.value();
    }
    void SetID(uint32_t InID) {
        ID = InID;
    }
protected:
    std::optional<uint32_t> ID;
    std::string Method;
    std::string Params;
};
class RPC_PARSER_EXPORT RPCResponse {
public:
    virtual ~RPCResponse() = default;
    virtual CharBuffer ToBytes() = 0;
    virtual bool IsError()const=0;
    std::string_view GetResult()const {
        return Result;
    }
    std::optional<uint32_t> ID;
    int64_t ErrorCode{ 0 };
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