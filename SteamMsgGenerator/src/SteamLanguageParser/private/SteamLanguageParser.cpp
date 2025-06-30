#include "SteamLanguageParser.h"

#include <string_convert.h>
#include <constant_hash.h>

#include <fstream>
#include <algorithm>
#include <string_view>
FSteamLanguageParser::FSteamLanguageParser() :re(TOKEN_PATTERN)
{
    IgnoredTypes.insert("EPersonaStateFlag");
    IgnoredTypes.insert("EPublishedFileInappropriateProvider");
    IgnoredTypes.insert("EPublishedFileInappropriateResult");
    IgnoredTypes.insert("EPublishedFileQueryType");
}
bool FSteamLanguageParser::Init(std::u8string_view projectDirView,std::u8string_view OutDirView)
{
    if (!re.ok()) {
        printf("%s", re.error().c_str());
        return false;
    }
    matchRes.assign(re.NamedCapturingGroups().size() + 1, absl::string_view());

    whitespaceIndex = re.NamedCapturingGroups().find("whitespace")->second;
    terminatorIndex = re.NamedCapturingGroups().find("terminator")->second;
    stringIndex = re.NamedCapturingGroups().find("string")->second;
    commentIndex = re.NamedCapturingGroups().find("comment")->second;
    identifierIndex = re.NamedCapturingGroups().find("identifier")->second;
    preprocessIndex = re.NamedCapturingGroups().find("preprocess")->second;
    operatorIndex = re.NamedCapturingGroups().find("operator")->second;
    invalidIndex = re.NamedCapturingGroups().find("invalid")->second;
    skipIndexs = { whitespaceIndex, commentIndex };
    skipIndexsSpan = skipIndexs;
    startSkipIndexs = { whitespaceIndex, commentIndex, terminatorIndex ,stringIndex ,operatorIndex };
    startSkipIndexsSpan = startSkipIndexs;
    OutDir = OutDirView;
    std::error_code ec;
    OutDir=std::filesystem::weakly_canonical(OutDir,  ec);
    if (ec) {
        return false;
    }
    ProjectDir = projectDirView;
    ProjectDir = std::filesystem::weakly_canonical(ProjectDir, ec);
    if (ec) {
        return false;
    }
    pCodeGenerator=ICodeGenerator::GetNamedClassSingleton(CPP_CODE_GEN_NAME);
    if (!pCodeGenerator) {
        return false;
    }

    return true;
}
bool FSteamLanguageParser::EmitEnumToOneFile(std::u8string_view enumFileView)
{
    std::error_code ec;
    EnumFilePath = enumFileView;
    ProjectDir = std::filesystem::weakly_canonical(ProjectDir, ec);
    if (ec) {
        return false;
    }
    return true;
}
std::tuple<bool, bool> FSteamLanguageParser::AddFileToParse(std::u8string_view path)
{
    auto [ptr, binsert] = AddFileToParseInternal(path);
    return { !!ptr,binsert };
}

bool FSteamLanguageParser::Parse()
{
    std::vector<std::u8string_view> fileNames;
    std::transform(FilesToParse.begin(), FilesToParse.end(), std::back_inserter(fileNames),
        [](const auto& pair) { return pair.first; });
    for (auto& fileName : fileNames) {
        if (!ParseFile(FilesToParse[fileName])) {
            return false;
        }
    }


    //for (size_t j = 0; j < match_result.size(); ++j) {
    //    if (match_result[j].matched) {
    //        Token_t token;
    //        token.Name = match_result[j].str();
    //        token.Value = match_result[j].str();
    //        token.FileName = ConvertStringToU8View(info.PathStr);
    //        // Assuming line and column numbers are not available in the regex match
    //        token.StartLineNumber = 0;
    //        token.StartColumnNumber = 0;
    //        token.EndLineNumber = 0;
    //        token.EndColumnNumber = 0;
    //        Tokens.push_back(token);
    //    }
    //}
    return true;
}

bool FSteamLanguageParser::EmitCode()
{
    std::error_code ec;
    NamespaceNode_t OutNSNode;
    NamespaceNode_t EnumNSNode;
    auto ItrNSNodeFunc = [&](this auto&& self, NamespaceNode_t& NSNode , NamespaceNode_t& OutNSNode, NamespaceNode_t& EnumNSNode)->void {
        for (auto type : NSNode.Types) {
            if (!type->bEmit) {
                continue;
            }
            auto pClassNode = dynamic_cast<FClassNode*>(type.get());
            auto pEnumNode = dynamic_cast<FEnumNode*>(type.get());
            if (pEnumNode) {
                if (!EnumFilePath.empty()) {
                    EnumNSNode.Types.push_back(type);
                }
                else {
                    OutNSNode.Types.push_back(type);
                }
            }
            else {
                OutNSNode.Types.push_back(type);
            }
        }

        for (auto& [ns, pNamespaceNode] : NSNode.SubNamespaces) {
            std::shared_ptr<NamespaceNode_t> pOutSubNSNode;
            auto tres= OutNSNode.SubNamespaces.try_emplace(ns, std::make_shared<NamespaceNode_t>());
            pOutSubNSNode = tres.first->second;
            if (tres.second) {
                pOutSubNSNode->Name = ns;
            }
            auto& OutSubNSNode = *pOutSubNSNode;

            std::shared_ptr<NamespaceNode_t> pEnumSubNSNode;
            tres = EnumNSNode.SubNamespaces.try_emplace(ns, std::make_shared<NamespaceNode_t>());
            pEnumSubNSNode = tres.first->second;
            if (tres.second) {
                pEnumSubNSNode->Name = ns;
            }
            auto& EnumSubNSNode = *pEnumSubNSNode;

            self(*pNamespaceNode, OutSubNSNode, EnumSubNSNode);
            if (OutSubNSNode.Types.size() <= 0&& OutSubNSNode.SubNamespaces.size()<=0) {
                OutNSNode.SubNamespaces.erase(ns);
            }
        }
        };
    for (const auto& [filePath, pFileInfo] : FilesToParse) {
        OutNSNode.SubNamespaces.clear();
        OutNSNode.Types.clear();
        ItrNSNodeFunc(pFileInfo->NamespaceNode, OutNSNode, EnumNSNode);
        if (OutNSNode.Types.size() <= 0&& OutNSNode.SubNamespaces.size()<=0) {
            continue;
        }
        auto outCPPPath= OutDir/pFileInfo->FilePath.lexically_relative(ProjectDir);
        outCPPPath.replace_extension(".h");
        std::filesystem::create_directories(outCPPPath.parent_path(), ec);
        if (ec) {
            return false;
        }
        std::ofstream ofs(outCPPPath,std::ios_base::trunc);
        if (!ofs.is_open()) {
            return false; // Failed to open output file
        }
        pCodeGenerator->EmitSourceFile(&ofs, pFileInfo->IncludeHeaders, OutNSNode);
    }
    if (!EnumFilePath.empty()) {
        std::ofstream ofs(EnumFilePath, std::ios_base::trunc);
        if (!ofs.is_open()) {
            return false; // Failed to open output file
        }
        std::vector<std::string> IncludeHeaders;
        pCodeGenerator->EmitSourceFile(&ofs, IncludeHeaders, EnumNSNode);
    }
    return true;
}

std::tuple<std::shared_ptr<SourceFileInfo_t>, bool> FSteamLanguageParser::AddFileToParseInternal(std::u8string_view path)
{
    std::error_code ec;
    auto pinfo = std::make_shared<SourceFileInfo_t>();
    SourceFileInfo_t& info = *pinfo;
    info.FilePath = std::filesystem::path(path).lexically_normal();
    info.FilePath = std::filesystem::weakly_canonical(info.FilePath);
    info.PathStr = ConvertU8ViewToString(info.FilePath.u8string());

    if (!std::filesystem::exists(info.FilePath, ec) || ec)
    {
        return { nullptr ,false }; // File does not exist or error occurred
    }
    auto index = info.FilePath.string().rfind(ProjectDir.string());
    if (index == std::string::npos) {
        return { nullptr ,false }; // File path does not contain current path
    }
    auto [pair, binsert] = FilesToParse.try_emplace(ConvertStringToU8View(pinfo->PathStr), pinfo);
    return { pinfo,binsert };
}

bool FSteamLanguageParser::ParseFile(std::shared_ptr<SourceFileInfo_t> pFileInfo)
{
    int i = 0;
    bool bres;
    auto& info = *pFileInfo;
    std::ifstream fileStream(info.FilePath, std::ios_base::binary | std::ios_base::ate);
    if (!fileStream.is_open()) {
        return false;
    }
    auto fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios_base::beg);
    info.FileContent.reserve(fileSize);
    fileStream.read(info.FileContent.data(), fileSize);
    fileStream.close();
    info.FileContentView = std::string_view(info.FileContent.data(), fileSize);
    absl::string_view fileContentView = info.FileContentView;

    auto nsNode = std::make_shared< NamespaceNode_t>();
    nsNode->Name = "utilpp";
    auto tres=info.NamespaceNode.SubNamespaces.try_emplace(nsNode->Name, nsNode);
    auto itr = tres.first;

    nsNode = std::make_shared< NamespaceNode_t>();
    nsNode->Name = "steam";
    tres=itr->second->SubNamespaces.try_emplace(nsNode->Name, nsNode);
    itr = tres.first;
    auto& types = nsNode->Types;
    while (fileContentView.size() > 0) {
        bres = GetNextToken(fileContentView, startSkipIndexsSpan, invalidIndex);
        if (!bres) {
            break;
        }
        if (!matchRes[identifierIndex].empty()) {
            auto ptr = ParseType(fileContentView);
            if(!ptr){
                return false;
            }
            if (IgnoredTypes.find(ptr->Name) != IgnoredTypes.end()) {
                continue; // Skip ignored types
            }
            types.push_back(ptr);
        }
        else if (!matchRes[preprocessIndex].empty()) {
            auto& view = matchRes[preprocessIndex];
            switch (ctcrc32(view)) {
            case ctcrc32("import"): {
                if (!GetNextToken(fileContentView, skipIndexsSpan, stringIndex)) {
                    return false;
                }
                auto iPath = info.FilePath.parent_path().append(ConvertViewToU8View(matchRes[stringIndex]));
                auto [ptr, binsert] = AddFileToParseInternal(iPath.u8string());
                if (!ptr) {
                    return false;
                }
                auto filename=iPath.filename();
                filename.replace_extension(".h");
                pFileInfo->IncludeHeaders.push_back(filename.string());
                if (binsert) {
                    if (!ParseFile(ptr)) {
                        return false;
                    }
                }
                break;
            }
            }

        }
        else {
            return false;
        }
    }
    return true;
}

bool FSteamLanguageParser::GetNextToken(absl::string_view& fileContentView, std::span<uint8_t>& skipIndexs, uint8_t& requestIndex ,std::string_view requestValue)
{
    bool bres;
    uint64_t remainLen;
    auto oriFileContentView = fileContentView;
    while (true) {
        bres = re.Match(fileContentView, 0, fileContentView.size(), RE2::ANCHOR_START, matchRes.data(), re.NamedCapturingGroups().size() + 1);
        if (!bres) {
            return false;
        }
        remainLen = fileContentView.size() - matchRes[0].size();
        fileContentView = absl::string_view(fileContentView.data() + matchRes[0].size(), remainLen);
        bool bSkip{ false };
        for (auto& skipIndex : skipIndexs) {
            if (!matchRes[skipIndex].empty()) {
                bSkip = true;
                break;
            }
        }
        if (bSkip) {
            continue;
        }
        if (requestIndex != invalidIndex) {
            if (matchRes[requestIndex].empty()) {
                fileContentView = oriFileContentView;
                return false;
            }
            else if (!requestValue.empty() && requestValue != matchRes[requestIndex]) {
                fileContentView = oriFileContentView;
                return false;
            }
        }
        return true;
    }
}

inline std::shared_ptr<FTypeNode> FSteamLanguageParser::ParseType(absl::string_view& fileContentView)
{
    bool bres;
    auto view = matchRes[identifierIndex];
    switch (ctcrc32(view)) {
    case ctcrc32("class"): {
        auto pClassNode = std::make_shared<FClassNode>();
        auto& classNode = *pClassNode;
        if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
            return nullptr;
        }
        classNode.Name = matchRes[identifierIndex];
        bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "<");
        if (bres) {
            if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
                return nullptr;
            }
            classNode.Identifier = matchRes[identifierIndex];
            if (!GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, ">")) {
                return nullptr;
            }
        }
        //bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex, "expects");
        //if (bres) {
        //    classNode.Parent=
        //}
        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex, "removed");
        if (bres) {
            classNode.bEmit = false;
        }
        if (!ParseTypeInner(fileContentView, pClassNode)) {
            return nullptr;
        }
        return pClassNode;
        break;
    }
    case ctcrc32("enum"): {
        auto pNode = std::make_shared<FEnumNode>();
        auto& node = *pNode;
        if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
            return nullptr;
        }
        node.Name = matchRes[identifierIndex];
        bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "<");
        if (bres) {
            if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
                return nullptr;
            }
            node.Type = ParseSteamTypeToCpp(matchRes[identifierIndex]);
            if (!GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, ">")) {
                return nullptr;
            }
        }

        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex, "flags");
        if (bres) {
            node.bFlag = true;
        }
        if(!ParseTypeInner(fileContentView, pNode)) {
            return nullptr;
        }
        return pNode;
        break;
    }
    case ctcrc32("public"): {
        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex);
        if (!bres) {
            return nullptr;
        }
        return ParseType(fileContentView);
    }
    }
    return nullptr;
}

inline bool FSteamLanguageParser::ParseTypeInner(absl::string_view& fileContentView, std::shared_ptr<FTypeNode> node)
{
    bool bres;
    if (!GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "{")) {
        return false;
    }
    while (true) {
        bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "}");
        if (bres) {
            break;
        }
        auto pPropNode = std::make_shared<FPropNode>();
        auto& propNode = *pPropNode;
        if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
            return false;
        }
        auto t1 = matchRes[identifierIndex];
        bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "<");
        if (bres) {
            if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
                return false;
            }
            propNode.FlagsOpt = matchRes[identifierIndex];
            if (!GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, ">")) {
                return false;
            }
        }
        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex);
        if (bres) {
            auto t2 = matchRes[identifierIndex];
            bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex);
            if (bres) {
                auto t3 = matchRes[identifierIndex];
                propNode.Flags = t1;
                propNode.Type = t2;
                propNode.Name = t3;
            }
            else {
                propNode.Type = t1;
                propNode.Name = t2;
            }
        }
        else {
            propNode.Name = t1;
        }
        bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "=");
        if (bres) {
            while (true) {
                if (!GetNextToken(fileContentView, skipIndexsSpan, identifierIndex)) {
                    return false;
                }
                propNode.DefaultValue.push_back(matchRes[identifierIndex]);
                bres = GetNextToken(fileContentView, skipIndexsSpan, operatorIndex, "|");
                if (!bres) {
                    break;
                }
            }
        }
        if (!GetNextToken(fileContentView, skipIndexsSpan, terminatorIndex)) {
            return false;
        }
        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex, "obsolete");
        if (bres) {
            auto t = matchRes[identifierIndex];
            bres = GetNextToken(fileContentView, skipIndexsSpan, stringIndex);
            propNode.bObsolete = true;
            if (bres) {
                propNode.ObsoleteReason = matchRes[stringIndex];
            }
        }
        bres = GetNextToken(fileContentView, skipIndexsSpan, identifierIndex, "removed");
        if (bres) {
            auto t = matchRes[identifierIndex];
            bres = GetNextToken(fileContentView, skipIndexsSpan, stringIndex);
            propNode.bEmit = false;
        }
        node->Children.push_back(pPropNode);
    }
    return true;
}
