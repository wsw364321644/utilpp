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
    virtual void ToBytes(FCharBuffer&) = 0;
    void SetParams(const char* cstr) {
        Params.Assign(cstr, strlen(cstr));
    }
    void SetParams(std::string_view view) {
        Params.Assign(view.data(), view.size());
    }
    std::string_view GetParams()const {
        return std::string_view(Params.CStr(),Params.Length());
    }
    FCharBuffer& GetParamsBuf(){
        return Params;
    }

    void SetMethod(std::string_view str) {
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
    FCharBuffer Params;
};
class RPC_PARSER_EXPORT RPCResponse {
public:
    virtual ~RPCResponse() = default;
    virtual void ToBytes(FCharBuffer&) = 0;
    virtual bool IsError()const=0;
    void SetResult(const char* cstr) {
        Result.Assign(cstr, strlen(cstr));
    }
    void SetResult(std::string_view view) {
        Result.Assign(view.data(), view.size());
    }
    std::string_view GetResult()const {
        return std::string_view(Result.CStr(), Result.Length());
    }
    FCharBuffer& GetResultBuf() {
        return Result;
    }

    void SetErrorMsg(std::string_view str) {
        ErrorMsg = str;
    }
    void SetErrorMsg(const char* cstr) {
        ErrorMsg.assign(cstr);
    }
    std::string_view GetErrorMsg()const {
        return ErrorMsg;
    }

    void SetErrorData(std::string_view str) {
        ErrorData = str;
    }
    void SetErrorData(const char* cstr) {
        ErrorData.assign(cstr);
    }
    std::string_view GetErrorData()const {
        return ErrorData;
    }

    bool HasID() const {
        return ID.has_value();
    }
    uint32_t GetID() const {
        return ID.value();
    }
    void SetID(uint32_t InID) {
        ID = InID;
    }
    void SetID(const std::optional<uint32_t>& InID) {
        ID = InID;
    }

    int64_t ErrorCode{ 0 };
protected:
    std::optional<uint32_t> ID;
    
    std::string ErrorMsg;
    std::string ErrorData;
    FCharBuffer Result;
};
class IRPCPaser {
public:
    typedef std::variant<ERPCParseError, std::shared_ptr<RPCRequest>, std::shared_ptr<RPCResponse>> ParseResult;
    virtual ParseResult Parse(const char* data, int len) = 0;
    virtual std::shared_ptr<RPCResponse> GetMethodNotFoundResponse(std::optional<uint32_t> id) = 0;
    virtual std::shared_ptr<RPCResponse> GetErrorParseResponse(ERPCParseError error) = 0;
};

#pragma warning(pop)