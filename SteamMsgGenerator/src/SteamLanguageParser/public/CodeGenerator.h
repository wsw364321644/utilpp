#pragma once 
#include "LanguageParser.h"
#include <handle.h>
#include <named_class_register.h>

#include <iostream>

extern const char CPP_CODE_GEN_NAME[];
class ICodeGenerator:public TNamedClassRegister<ICodeGenerator> {
public:
    virtual void EmitSourceFile(std::ostream* stream, std::vector<std::string>& IncludeHeaders, NamespaceNode_t& Types) = 0;
    virtual FCommonHandlePtr BeginNamespace(std::ostream* stream, std::string_view spaceName) = 0;
    virtual void EmitType(std::ostream* stream, FTypeNode* type) = 0;

    virtual bool SupportsNamespace() = 0;
    virtual bool SupportsUnsignedTypes() = 0;
};
