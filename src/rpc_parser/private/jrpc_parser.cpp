#include "jrpc_parser.h"

#include "JsonRPCError.h"
#include <LoggerHelper.h>
#include <std_ext.h>
#include <delegate_macros.h>
#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <filesystem>
#include <fstream>
#include <nlohmann/json-schema.hpp>

const char JsonVersionStr[] = "2.0";
const char SpecificationStr[] = "specification";
const char MethodFieldStr[] = "method";
const char ParamsFieldStr[] = "params";
const char ResultFieldStr[] = "result";
const char IDFieldStr[] = "id";
const char ErrorFieldStr[] = "error";
const char ErrorCodeFieldStr[] = "code";
const char ErrorMsgFieldStr[] = "message";
const char ErrorDataFieldStr[] = "data";
const char* ReqSchemaStr = R"(
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "method": {
      "type": "string"
    },
    "params": {
      "not": {
          "type": "null"
        }
    },
    "id": {
      "oneOf": [
        {
          "type": "number"
        },
        {
          "type": "string"
        },
        {
          "type": "null"
        }
      ]
    }
  },
  "required": [
    "method"
  ]
}
)";

const char* RespondSchemaStr = R"(
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "result": {},
    "error": {
      "type": "object",
      "properties": {
        "code": {
          "type": "number"
        },
        "message": {
          "type": "string"
        },
        "data": {
          "not": {
            "type": "null"
          }
        }
      },
      "required": [
        "code",
        "message"
      ]
    },
    "id": {
      "oneOf": [
        {
          "type": "number"
        },
        {
          "type": "string"
        },
        {
          "type": "null"
        }
      ]
    }
  },
  "oneOf": [
    {
      "required": [
        "result",
        "id"
      ]
    },
    {
      "required": [
        "error",
        "id"
      ]
    }
  ]
}
)";

static nlohmann::json_schema::json_validator reqValidator;
static nlohmann::json_schema::json_validator respValidator;


ERPCParseError JsonRPCResponse::CheckResult(const char* Method) const
{
    auto path = JRPCPaser::GetResultFilePath(Method);
    if (!std::filesystem::exists(path)) {
        return ERPCParseError::OK;
    }
    std::ifstream ifs(path, std::ios::binary);
    auto schemaDoc=nlohmann::json::parse(ifs, nullptr, false);
    if (schemaDoc.is_discarded()) {
        return  ERPCParseError::InternalError;
    }
    nlohmann::json_schema::json_validator validator;
    nlohmann::json_schema::basic_error_handler err;
    try {
        validator.set_root_schema(schemaDoc);
    }
    catch (const std::exception& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr,"Validation of {} Result failed, here is why: {}", Method, e.what());
        return  ERPCParseError::InternalError;;
    }
    validator.validate(GetResultNlohmannJson(), err);
    if (err) {
        return  ERPCParseError::ParseError;
    }
    return ERPCParseError::OK;
}

CharBuffer JsonRPCResponse::ToBytes()
{
    return JRPCPaser::ToByte(*this);
}

ERPCParseError JsonRPCRequest::CheckParams()const
{
    auto path = JRPCPaser::GetParamsFilePath(Method.c_str());
    if (!std::filesystem::exists(path)) {
        return ERPCParseError::OK;
    }
    std::ifstream ifs(path, std::ios::binary);
    auto schemaDoc = nlohmann::json::parse(ifs, nullptr, false);
    if (schemaDoc.is_discarded()) {
        return  ERPCParseError::InternalError;
    }
    nlohmann::json_schema::json_validator validator;
    nlohmann::json_schema::basic_error_handler err;
    try {
        validator.set_root_schema(schemaDoc);
    }
    catch (const std::exception& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "Validation of {} Params failed, here is why: {}", Method, e.what());
        return  ERPCParseError::InternalError;;
    }
    validator.validate(GetParamsNlohmannJson(), err);
    if (err) {
        return  ERPCParseError::ParseError;
    }
    return ERPCParseError::OK;
}

CharBuffer JsonRPCRequest::ToBytes()
{
    return JRPCPaser::ToByte(*this);
}


std::unordered_map<std::string, RPCInfo_t>  JRPCPaser::rpcFuncMap;

bool JRPCPaser::bInited = JRPCPaser::Init();
bool JRPCPaser::Init()
{
    auto reqSchema=nlohmann::json::parse(ReqSchemaStr, nullptr, false);
    if (reqSchema.is_discarded()) {
        return false;
    }
    try {
        reqValidator.set_root_schema(reqSchema);
    }
    catch (const std::exception& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "ReqSchema init failed, here is why: {}", e.what());
        return false;
    }

    auto respSchema = nlohmann::json::parse(RespondSchemaStr, nullptr, false);
    if (respSchema.is_discarded()) {
        return false;
    }
    try {
        respValidator.set_root_schema(respSchema);
    }
    catch (const std::exception& e) {
        SIMPLELOG_LOGGER_ERROR(nullptr, "RespSchema init failed, here is why: {}", e.what());
        return false;
    }
    //todo
    //std::filesystem::path spepath = std::filesystem::current_path() / SpecificationStr;
    //if (!std::filesystem::exists(spepath)) {
    //    return false;
    //}

    //for (int i = 0; i < (int)ERPCType::count; i++) {
    //    ERPCType curtype = (ERPCType)i;
    //    std::filesystem::path rpcpath = spepath / RPCTypeToString(curtype);

    //    //auto pair = rpcFuncMap.emplace(curtype, std::initializer_list<std::string>{});
    //    //auto& rpcs=pair.first->second;
    //    if (!std::filesystem::exists(rpcpath)) {
    //        return false;
    //    }
    //    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(rpcpath)) {
    //        if (!dirEntry.is_directory()) {
    //            continue;
    //        }
    //        auto fn = dirEntry.path().filename();
    //        auto result = rpcFuncMap.emplace(fn.string(), RPCInfo_t{ .type = curtype });
    //        if (!result.second) {
    //            return false;
    //        }

    //        //if (dirEntry.is_directory()) {
    //        //    continue;
    //        //}
    //        //auto fn = dirEntry.path().filename();
    //        //if (fn.extension() == ".json") {
    //        //    fn.replace_extension();
    //        //    rpcs.insert(fn.string());
    //        //}
    //    }
    //}


    return true;
}
JRPCPaser::ParseResult JRPCPaser::Parse(const char* data, int len)
{
    return StaticParse(data, len);
}

std::shared_ptr<RPCResponse> JRPCPaser::GetMethodNotFoundResponse(std::optional<uint32_t> id)
{
    auto presponse = std::make_shared<JsonRPCResponse>();
    auto& response = *presponse;
    response.ID = id;
    response.ErrorCode = std::to_underlying( EJsonRPCError::MethodNotFound);
    response.ErrorMsg = ToString(EJsonRPCError::MethodNotFound);
    response.OptError = true;
    return  presponse;
}

std::shared_ptr<RPCResponse> JRPCPaser::GetErrorParseResponse(ERPCParseError error)
{
    auto presponse = std::make_shared<JsonRPCResponse>();
    auto& response = *presponse;
    response.OptError = true;
    switch (error)
    {
    case ERPCParseError::ParseError:
        response.ErrorCode = std::to_underlying(EJsonRPCError::ParseError);
        response.ErrorMsg = ToString(EJsonRPCError::ParseError);
        break;
    case ERPCParseError::InternalError:
        response.ErrorCode = std::to_underlying(EJsonRPCError::InternalError);
        response.ErrorMsg = ToString(EJsonRPCError::InternalError);
        break;
    case ERPCParseError::InvalidRequest:
        response.ErrorCode = std::to_underlying(EJsonRPCError::InvalidRequest);
        response.ErrorMsg = ToString(EJsonRPCError::InvalidRequest);
        break;
    default:
        return nullptr;
        break;
    }
    return presponse;
}

JRPCPaser::ParseResult JRPCPaser::StaticParse(const char* data, int len)
{
    ParseResult res(ERPCParseError::ParseError);
    std::string_view view(data, len);
    auto doc=nlohmann::json::parse(view, nullptr, false);
    if (doc.is_discarded()) {
        return res;
    }
    res = ERPCParseError::InvalidRequest;
    nlohmann::json_schema::basic_error_handler err;
    reqValidator.validate(doc, err);
    if (!err) {
        auto preq = std::make_shared<JsonRPCRequest>();
        if (doc.find(IDFieldStr)!=doc.end() && doc[IDFieldStr].is_number()) {
            preq->ID.emplace((uint32_t)doc[IDFieldStr].get_ref<nlohmann::json::number_unsigned_t&>());
        }
        preq->Method = doc[MethodFieldStr].get_ref<nlohmann::json::string_t&>();
        if (doc.find(ParamsFieldStr)!=doc.end()) {
            preq->Params = doc[ParamsFieldStr].dump();
        }
        return preq;
    }

    err.reset();
    respValidator.validate(doc, err);
    if (!err) {
        auto presponse = std::make_shared<JsonRPCResponse>();
        auto& response = *presponse;
        if (doc[IDFieldStr].is_number()) {
            response.ID = (uint32_t)doc[IDFieldStr].get_ref<nlohmann::json::number_unsigned_t&>();
        }

        if (doc.find(ResultFieldStr)!=doc.end()) {
            response.OptError = false;
            response.Result = doc[ResultFieldStr].dump();
        }
        else {
            response.OptError = true;;
            response.ErrorCode = doc[ErrorFieldStr][ErrorCodeFieldStr].get_ref<nlohmann::json::number_integer_t&>();
            response.ErrorMsg = doc[ErrorFieldStr][ErrorMsgFieldStr].get_ref<nlohmann::json::string_t&>();
            if (doc[ErrorFieldStr].find(ErrorDataFieldStr)!= doc[ErrorFieldStr].end()) {
                response.ErrorData= doc[ErrorFieldStr][ErrorDataFieldStr].get_ref<nlohmann::json::string_t&>();
            }
        }
        return presponse;
    }
    return res;
}

CharBuffer JRPCPaser::ToByte(const JsonRPCRequest& req)
{
    CharBuffer buffer;
    nlohmann::json doc(nlohmann::json::value_t::object);
    doc[MethodFieldStr] = req.Method;
    auto Paramsnode=req.GetParamsNlohmannJson();
    if (!Paramsnode.empty()) {
        doc[ParamsFieldStr] = Paramsnode;
    }
    if (req.ID.has_value()) {
        doc[IDFieldStr]= req.ID.value();
    }
    else {
        doc[IDFieldStr] = nlohmann::json(nlohmann::json::value_t::null);
    }
    auto str=doc.dump();
    buffer.Assign(str.c_str(), str.size());
    return buffer;
}

CharBuffer JRPCPaser::ToByte(const JsonRPCResponse& res)
{
    CharBuffer buffer;
    nlohmann::json doc(nlohmann::json::value_t::object);
    if (!res.IsValiad()) {
        return buffer;
    }
    if (res.ID.has_value()) {
        doc[IDFieldStr] = res.ID.value();
    }
    else {
        doc[IDFieldStr] = nlohmann::json(nlohmann::json::value_t::null);
    }
    if (res.IsError()) {
        nlohmann::json errNode(nlohmann::json::value_t::object);
        errNode[ErrorCodeFieldStr] = res.ErrorCode;
        errNode[ErrorMsgFieldStr]=res.ErrorMsg;
        if (!res.ErrorData.empty()) {
            errNode[ErrorDataFieldStr]= res.ErrorData;
        }
        doc[ErrorFieldStr] = errNode;
    }
    else {
        auto resNode = res.GetResultNlohmannJson();
        doc[ResultFieldStr]= resNode;
    }
    auto str = doc.dump();
    buffer.Assign(str.c_str(), str.size());
    return buffer;
}

std::string JRPCPaser::RPCTypeToString(ERPCType type)
{
    std::string res;
    switch (type)
    {
    case ERPCType::command:
        return "command";
        break;
    case ERPCType::apicommon:
        return "apicommon";
        break;
    default:
        break;
    }
    return res;
}



std::filesystem::path JRPCPaser::GetParamsFilePath(const char* method) {
    auto pair = rpcFuncMap.find(method);
    if (pair == rpcFuncMap.end()) {
        return std::filesystem::path();
    }
    return std::filesystem::current_path() / SpecificationStr / JRPCPaser::RPCTypeToString(pair->second.type) / method / (std::string(ParamsFieldStr) + ".json");
}
std::filesystem::path JRPCPaser::GetResultFilePath(const char* method) {
    auto pair = rpcFuncMap.find(method);
    if (pair == rpcFuncMap.end()) {
        return std::filesystem::path();
    }
    return std::filesystem::current_path() / SpecificationStr / JRPCPaser::RPCTypeToString(pair->second.type) / method / (std::string(ResultFieldStr) + ".json");
}
std::filesystem::path JRPCPaser::GetErrorFilePath(const char* method) {
    auto pair = rpcFuncMap.find(method);
    if (pair == rpcFuncMap.end()) {
        return std::filesystem::path();
    }
    return std::filesystem::current_path() / SpecificationStr / JRPCPaser::RPCTypeToString(pair->second.type) / method / (std::string(ErrorFieldStr) + ".json") ;
}