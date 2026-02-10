#pragma once 
#include "LanguageParser.h"
#include <CPPCodeGenerator.h>
#include <constant_hash.h>
#include <re2/re2.h>
#include <unordered_map>
#include <span>
#include <variant>
#include <unordered_set>

constexpr char TOKEN_PATTERN[] = R"((?m)(?<whitespace>\s+)|(?<terminator>[;])|[\"](?<string>.+?)[\"]|\/\/(?<comment>.*)$|(?<identifier>-?[a-zA-Z_0-9][a-zA-Z0-9_:.]*)|[#](?<preprocess>[a-zA-Z]*)|(?<operator>[{}<>\]=|])|(?<invalid>[^\s]+))";

inline std::variant<EBuildInType, std::string_view> ParseSteamTypeToCpp(std::string_view typeStr) {
    switch  (ctcrc32(typeStr)) {
    case ctcrc32("uint"): return EBuildInType::BIT_UINT32;
    case ctcrc32("byte"): return EBuildInType::BIT_UINT8;
    } 
    return typeStr;
}

class FSteamLanguageParser {
public:
    FSteamLanguageParser();
    bool Init(std::u8string_view projectDirView, std::u8string_view OutDirView);
    bool EmitEnumToOneFile(std::u8string_view enumFileView);
    std::tuple<bool,bool> AddFileToParse(std::u8string_view path);
    bool Parse();
    bool EmitCode();

    std::unordered_map<std::u8string_view,std::shared_ptr<SourceFileInfo_t>> FilesToParse;
    std::filesystem::path OutDir;
    std::filesystem::path ProjectDir;
    std::filesystem::path EnumFilePath;
    std::shared_ptr <ICodeGenerator> pCodeGenerator;
    std::unordered_set<std::string_view, string_hash> IgnoredTypes;
private:
    std::tuple<std::shared_ptr<SourceFileInfo_t>, bool> AddFileToParseInternal(std::u8string_view path);
    bool ParseFile(std::shared_ptr<SourceFileInfo_t> pFileInfo);
    inline bool GetNextToken(absl::string_view& fileContentView,std::span<uint8_t>& skipIndexs, uint8_t& requestIndex, std::string_view requestValue = std::string_view());
    inline std::shared_ptr<FTypeNode> ParseType(absl::string_view& fileContentView);
    inline bool ParseTypeInner(absl::string_view& fileContentView, std::shared_ptr<FTypeNode>);
    //cache
    inline static thread_local std::vector<absl::string_view> matchRes;
    RE2 re;
    uint8_t whitespaceIndex;
    uint8_t terminatorIndex;
    uint8_t stringIndex;
    uint8_t commentIndex;
    uint8_t identifierIndex;
    uint8_t preprocessIndex;
    uint8_t operatorIndex;
    uint8_t invalidIndex;
    std::vector<uint8_t> startSkipIndexs;
    std::span<uint8_t> startSkipIndexsSpan;
    std::vector<uint8_t> skipIndexs;
    std::span<uint8_t> skipIndexsSpan;
};