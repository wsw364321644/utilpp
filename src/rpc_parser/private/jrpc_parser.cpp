#include "jrpc_parser.h"

#include "JsonRPCError.h"
#include <LoggerHelper.h>
#include <std_ext.h>
#include <string_convert.h>
#include <FunctionExitHelper.h>

#include <memory>
#include <stdint.h>
#include <shared_mutex>
#include <filesystem>
#include <fstream>

#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/schema.h>

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

static rapidjson::Document tempDoc;
static rapidjson::SchemaDocument reqSchemaDocument(tempDoc);
static rapidjson::SchemaValidator reqValidator(reqSchemaDocument);
static rapidjson::SchemaDocument respSchemaDocument(tempDoc);
static rapidjson::SchemaValidator respValidator(respSchemaDocument);
//static thread_local simdjson::ondemand::parser simdjsonParser;
static thread_local std::vector<char> threadBuf;
static thread_local rapidjson::StringBuffer rapidjsonBuf;

ERPCParseError JsonRPCResponse::CheckResult(const char* Method) const
{
    auto path = JRPCPaser::GetResultFilePath(Method);
    if (!std::filesystem::exists(path)) {
        return ERPCParseError::OK;
    }
    auto pathStru8 = path.u8string();
    FILE* fp = fopen(ConvertU8StringToView(pathStru8).data(), "rb");
    if (!fp) {
        return ERPCParseError::InternalError;
    }
    FunctionExitHelper_t fpHelper(
        [&]() {
            fclose(fp);
        }
    );
    threadBuf.reserve(1<<16);
    rapidjson::Document sdoc;
    rapidjson::FileReadStream is(fp, threadBuf.data(), threadBuf.capacity());
    sdoc.ParseStream(is);
    if (sdoc.HasParseError()) {
        return ERPCParseError::ParseError;
    }
    rapidjson::SchemaDocument schemaDoc(sdoc);
    rapidjson::SchemaValidator validator(schemaDoc);
    rapidjson::Document rdoc;
    rdoc.Parse(GetResult().data());
    if (rdoc.HasParseError()) {
        return ERPCParseError::InvalidRequest;
    }
    if (!rdoc.Accept(validator)) {
        return ERPCParseError::InvalidRequest;
    }
    return ERPCParseError::OK;
}

void JsonRPCResponse::ToBytes(FCharBuffer& buf)
{
    return JRPCPaser::ToByte(*this, buf);
}

ERPCParseError JsonRPCRequest::CheckParams()const
{
    auto path = JRPCPaser::GetParamsFilePath(Method.c_str());
    if (!std::filesystem::exists(path)) {
        return ERPCParseError::OK;
    }
    auto pathStru8 = path.u8string();
    FILE* fp = fopen(ConvertU8StringToView(pathStru8).data(), "rb");
    if (!fp) {
        return ERPCParseError::InternalError;
    }
    FunctionExitHelper_t fpHelper(
        [&]() {
            fclose(fp);
        }
    );
    threadBuf.reserve(1 << 16);
    rapidjson::Document sdoc;
    rapidjson::FileReadStream is(fp, threadBuf.data(), threadBuf.capacity());
    sdoc.ParseStream(is);
    if (sdoc.HasParseError()) {
        return ERPCParseError::ParseError;
    }
    rapidjson::SchemaDocument schemaDoc(sdoc);
    rapidjson::SchemaValidator validator(schemaDoc);
    rapidjson::Document doc;
    doc.Parse(GetParams().data());
    if (doc.HasParseError()) {
        return ERPCParseError::InvalidRequest;
    }
    if (!doc.Accept(validator)) {
        return ERPCParseError::InvalidRequest;
    }
    return ERPCParseError::OK;
}

void JsonRPCRequest::ToBytes(FCharBuffer& buf)
{
    return JRPCPaser::ToByte(*this, buf);
}


std::unordered_map<std::string, RPCInfo_t>  JRPCPaser::rpcFuncMap;

bool JRPCPaser::bInited = JRPCPaser::Init();
bool JRPCPaser::Init()
{
    tempDoc.Parse(ReqSchemaStr);
    if (tempDoc.HasParseError()) {
        return false;
    }
    new(&reqSchemaDocument) rapidjson::SchemaDocument(tempDoc);
    new(&reqValidator) rapidjson::SchemaValidator(reqSchemaDocument);

    tempDoc.Parse(RespondSchemaStr);
    if (tempDoc.HasParseError()) {
        return false;
    }
    new(&respSchemaDocument) rapidjson::SchemaDocument(tempDoc);
    new(&respValidator) rapidjson::SchemaValidator(respSchemaDocument);

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
    response.SetID(id);
    response.ErrorCode = std::to_underlying( EJsonRPCError::MethodNotFound);
    response.SetErrorMsg(ToString(EJsonRPCError::MethodNotFound));
    response.SetError( true);
    return  presponse;
}

std::shared_ptr<RPCResponse> JRPCPaser::GetErrorParseResponse(ERPCParseError error)
{
    auto presponse = std::make_shared<JsonRPCResponse>();
    auto& response = *presponse;
    response.SetError(true);
    switch (error)
    {
    case ERPCParseError::ParseError:
        response.ErrorCode = std::to_underlying(EJsonRPCError::ParseError);
        response.SetErrorMsg(ToString(EJsonRPCError::ParseError));
        break;
    case ERPCParseError::InternalError:
        response.ErrorCode = std::to_underlying(EJsonRPCError::InternalError);
        response.SetErrorMsg(ToString(EJsonRPCError::InternalError));
        break;
    case ERPCParseError::InvalidRequest:
        response.ErrorCode = std::to_underlying(EJsonRPCError::InvalidRequest);
        response.SetErrorMsg(ToString(EJsonRPCError::InvalidRequest));
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
    rapidjson::Document doc;
    doc.Parse(data, len);
    if (doc.HasParseError()) {
        return res;
    }
    res = ERPCParseError::InvalidRequest;
    reqValidator.Reset();
    respValidator.Reset();
    if (doc.Accept(reqValidator)) {
        auto preq = std::make_shared<JsonRPCRequest>();
        if (doc.HasMember(IDFieldStr)&& doc[IDFieldStr].IsNumber()) {
            preq->SetID(doc[IDFieldStr].GetUint());
        }
        if (doc.HasMember(ParamsFieldStr)) {
            rapidjson::Writer<FCharBuffer> writer(preq->GetParamsBuf());
            if (!doc[ParamsFieldStr].Accept(writer)) {
                return res;
            }
        }
        preq->SetMethod(doc[MethodFieldStr].GetString());
        return preq;
    }
    else if (doc.Accept(respValidator)) {
        auto presponse = std::make_shared<JsonRPCResponse>();
        auto& response = *presponse;
        if (doc[IDFieldStr].IsNumber()) {
            response.SetID(doc[IDFieldStr].GetUint());
        }
        if (doc.HasMember(ResultFieldStr)) {
            response.SetError(false);
            rapidjsonBuf.Clear();
            rapidjson::Writer<rapidjson::StringBuffer> writer(rapidjsonBuf);
            if (doc[ResultFieldStr].Accept(writer)) {
                response.SetResult(rapidjsonBuf.GetString());
            }
        }
        else {
            response.SetError(true);
            response.ErrorCode = doc[ErrorFieldStr][ErrorCodeFieldStr].GetInt64();
            response.SetErrorMsg(doc[ErrorFieldStr][ErrorMsgFieldStr].GetString());
            if (doc[ErrorFieldStr].HasMember(ErrorDataFieldStr)) {
                response.SetErrorData(doc[ErrorFieldStr][ErrorDataFieldStr].GetString());
            }
        }
        return presponse;
    }
    //simdjsonBuf.reserve(len+ simdjson::SIMDJSON_PADDING);
    //memcpy(simdjsonBuf.data(), data, len);
    //simdjson::ondemand::document doc = simdjsonParser.iterate(data, len, simdjsonBuf.capacity());
    return res;
}

void JRPCPaser::ToByte(const JsonRPCRequest& req, FCharBuffer& buf)
{
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& a = doc.GetAllocator();
    doc.AddMember(MethodFieldStr, rapidjson::StringRef(req.GetMethod().data(), req.GetMethod().size()), a);
    if (req.ID.has_value()) {
        doc.AddMember(IDFieldStr, req.ID.value(), a);
    }
    else {
        doc.AddMember(IDFieldStr, rapidjson::Value(rapidjson::Type::kNullType), a);
    }
    if (!req.GetParams().empty()) {
        rapidjson::Document paramsNode(&a);
        paramsNode.Parse(req.GetParams().data());
        if (paramsNode.HasParseError()) {
            return;
        }
        doc.AddMember(ParamsFieldStr, paramsNode, a);
    }
    buf.Clear();
    rapidjson::Writer<FCharBuffer> writer(buf);
    doc.Accept(writer);
    return;
}

void JRPCPaser::ToByte(const JsonRPCResponse& res, FCharBuffer& buf)
{
    rapidjson::Document doc(rapidjson::kObjectType);
    auto& allocator = doc.GetAllocator();
    if (!res.IsValiad()) {
        return;
    }
    if (res.HasID()) {
        doc.AddMember(IDFieldStr, res.GetID(), allocator);
    }
    else {
        doc.AddMember(IDFieldStr, rapidjson::Value(rapidjson::Type::kNullType), allocator);
    }
    if (res.IsError()) {
        rapidjson::Value errNode(rapidjson::kObjectType);
        errNode.AddMember(ErrorCodeFieldStr, res.ErrorCode, allocator);
        errNode.AddMember(ErrorMsgFieldStr, rapidjson::StringRef(res.GetErrorMsg().data(), res.GetErrorMsg().size()), allocator);
        if (!res.GetErrorData().empty()) {
            errNode.AddMember(ErrorDataFieldStr, rapidjson::StringRef(res.GetErrorData().data(), res.GetErrorData().size()), allocator);
        }
        doc.AddMember(ErrorFieldStr, errNode, allocator);
    }
    else {
        rapidjson::Document resNode(&allocator);
        resNode.Parse(res.GetResult().data());
        if (resNode.HasParseError()) {
            return;
        }
        doc.AddMember(ResultFieldStr, resNode, allocator);
    }
    buf.Clear();
    rapidjson::Writer<FCharBuffer> writer(buf);
    doc.Accept(writer);
    return;
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