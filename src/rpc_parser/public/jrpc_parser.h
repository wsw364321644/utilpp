#pragma once

#include <unordered_map>
#include <filesystem>
#include <optional>
#include "rpc_definition.h"

//#include <nlohmann/json.hpp>
//#define GetParamsNlohmannJson(Request) nlohmann::json::parse((Request).GetParams().data(), nullptr, false)
//#define GetResultNlohmannJson(Response) nlohmann::json::parse((Response).GetResult().data(), nullptr, false)

#pragma warning(push)
#pragma warning(disable:4251)

enum class  ERPCType :uint8_t {
    command,
    apicommon,
    count
};
struct RPCInfo_t {
    ERPCType type;
};

class RPC_PARSER_EXPORT  JsonRPCRequest :public RPCRequest {
    friend class JRPCPaser;
public:
    JsonRPCRequest() = default;
    ~JsonRPCRequest() override = default;
    JsonRPCRequest(const JsonRPCRequest& rhs) {
        *this = rhs;
    }
    bool IsValiad() {
        return !Method.empty();
    }
    JsonRPCRequest& operator=(const JsonRPCRequest& rhs) {
        Method = rhs.Method;
        ID = rhs.ID;
        Params = rhs.Params;
        return *this;
    }
    JsonRPCRequest& operator=(JsonRPCRequest&& rhs) noexcept {
        Method = rhs.Method;
        ID = rhs.ID;
        Params = std::move(rhs.Params);
        return *this;
    }
    ERPCParseError CheckParams()const;
    void ToBytes(FCharBuffer&) override;

};
class RPC_PARSER_EXPORT JsonRPCResponse :public RPCResponse {
public:
    bool IsValiad() const {
        return  OptError.has_value();
    };
    JsonRPCResponse() = default;
    ~JsonRPCResponse() override = default;
    JsonRPCResponse(const JsonRPCResponse& rhs) {
        *this = rhs;
    }
    JsonRPCResponse& operator=(const JsonRPCResponse& rhs) {
        //bSuccess = rhs.bSuccess;
        ID = rhs.ID;
        ErrorCode = rhs.ErrorCode;
        ErrorMsg = rhs.ErrorMsg;
        Result = rhs.Result;
        return *this;
    }
    JsonRPCResponse& operator=(JsonRPCResponse&& rhs)noexcept {
        //bSuccess = rhs.bSuccess;
        ID = rhs.ID;
        ErrorCode = rhs.ErrorCode;
        ErrorMsg = rhs.ErrorMsg;
        Result = std::move(rhs.Result);
        return *this;
    }

    ERPCParseError CheckResult(const char* Method) const;
    void ToBytes(FCharBuffer&) override;
    bool IsError()const override{
        return OptError.has_value()? OptError.value():true;
    }
    void SetError(bool InErr) {
        OptError = InErr;
    }
    void SetError(const std::optional<bool>& InErr) {
        OptError = InErr;
    }
private:
    std::optional<bool> OptError;
};
class RPC_PARSER_EXPORT JRPCPaser :public IRPCPaser
{
    friend class JsonRPCResponse;
    friend class JsonRPCRequest;
    friend class FJRPCProcesser;
private:
    JRPCPaser() = default;

public:
    ~JRPCPaser() = default;
    //typedef std::variant<EMessageError,JsonRPCRequest, JsonRPCResponse> ParseResult;
    static bool Init();
    ParseResult Parse(const char* data, int len) override;
    std::shared_ptr<RPCResponse> GetMethodNotFoundResponse(std::optional<uint32_t> id) override;
    std::shared_ptr<RPCResponse> GetErrorParseResponse(ERPCParseError error) override;
    static ParseResult StaticParse(const char* data, int len);
    static void ToByte(const JsonRPCRequest&, FCharBuffer&);
    static void ToByte(const JsonRPCResponse&, FCharBuffer&);
    static std::string RPCTypeToString(ERPCType type);
private:
    static std::filesystem::path GetParamsFilePath(const char* method);
    static std::filesystem::path GetResultFilePath(const char* method);
    static std::filesystem::path GetErrorFilePath(const char* method);

    static std::unordered_map<std::string, RPCInfo_t> rpcFuncMap;

    static bool bInited;
};
#pragma warning(pop)