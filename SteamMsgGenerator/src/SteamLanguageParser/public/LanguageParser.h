#pragma once 
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <filesystem>

enum class EBuildInType {
    BIT_NONE = 0,
    BIT_INT8,
    BIT_UINT8,
    BIT_INT16,
    BIT_UINT16,
    BIT_INT32,
    BIT_UINT32,
    BIT_INT64,
    BIT_UINT64,
    BIT_FLOAT,
    BIT_DOUBLE,
    BIT_PTR,
    BIT_BOOL,
    BIT_U8STRING,
    BIT_U16STRING,

};

typedef struct Token_t {
    std::string Name;
    std::string Value;
    std::u8string_view FilePatn;
    int32_t StartLineNumber;
    int32_t StartColumnNumber;
    int32_t EndLineNumber;
    int32_t EndColumnNumber;
} Token_t;

class FTypeNode;

class FNode {
public:
    virtual ~FNode() = default;
    bool bEmit{ true };
    bool bObsolete{ false };
    std::string_view ObsoleteReason;
};

class FPropNode :public FNode {
public:
    std::string_view Type;
    std::string_view Name;
    std::string_view Flags;
    std::string_view FlagsOpt;
    std::vector<std::string_view> DefaultValue;
};

class FTypeNode :public FNode {
public:
    std::vector<std::shared_ptr<FPropNode>> Children;
    std::string_view Name;
};

class FClassNode :public FTypeNode {
public:
    std::vector<std::shared_ptr<FTypeNode>> SubType;
    std::shared_ptr<FTypeNode> Parent;
    std::string_view Identifier;
};

class FEnumNode :public FTypeNode {
public:
    std::variant< EBuildInType, std::string_view> Type{ EBuildInType ::BIT_NONE};
    bool bFlag;
};

typedef struct SourceFileInfo_t {
    std::string PathStr;
    std::filesystem::path FilePath;
    std::vector<char> FileContent;
    std::string_view FileContentView;

    std::vector<std::string> IncludeHeaders;
    std::vector<std::shared_ptr<FTypeNode>> Types;
} SourceFileInfo_t;