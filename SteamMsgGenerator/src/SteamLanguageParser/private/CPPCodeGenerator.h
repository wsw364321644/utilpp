#pragma once 
#include "CodeGenerator.h"

inline std::string_view GetCPPBuildInTypeView(EBuildInType type) {
    switch (type) {
    case EBuildInType::BIT_INT8: return "int8_t";
    case EBuildInType::BIT_UINT8: return "uint8_t";
    case EBuildInType::BIT_INT16: return "int16_t";
    case EBuildInType::BIT_UINT16: return "uint16_t";
    case EBuildInType::BIT_INT32: return "int32_t";
    case EBuildInType::BIT_UINT32: return "uint32_t";
    case EBuildInType::BIT_INT64: return "int64_t";
    case EBuildInType::BIT_UINT64: return "uint64_t";
    case EBuildInType::BIT_FLOAT: return "float";
    case EBuildInType::BIT_DOUBLE: return "double";
    case EBuildInType::BIT_PTR: return "void*";
    case EBuildInType::BIT_BOOL: return "bool";
    case EBuildInType::BIT_U8STRING: return "char8_t";
    case EBuildInType::BIT_U16STRING: return "char16_t";
    }
    return "";

}

class FCPPCodeGenerator : public ICodeGenerator {
public:
    void EmitSourceFile(std::ostream* stream, std::vector<std::string>& IncludeHeaders, NamespaceNode_t& Types) override;
    FCommonHandlePtr BeginNamespace(std::ostream* stream, std::string_view spaceName) override;
    void EmitType(std::ostream* stream, FTypeNode* node) override;

    bool SupportsNamespace() override;
    bool SupportsUnsignedTypes() override;
};