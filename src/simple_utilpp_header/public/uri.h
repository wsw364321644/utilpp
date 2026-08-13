#pragma once
#include <string>
#include <regex>
typedef struct ParsedURL_t
{
    std::string_view outScheme;
    std::string_view outAuthority;
    std::string_view outPort;
    std::string_view outPath;
    std::vector<std::pair<std::string_view, std::string_view>> outQuery;
    std::string_view outFragment;
}ParsedURL_t;

inline std::string_view get_sv(const std::cmatch& m, int idx, const std::string_view base) {
    return std::string_view(base.data() + m.position(idx), m.length(idx));
}

inline void ParseUrl(std::string_view inurl, ParsedURL_t& ParsedURL) {
    //https://www.rfc-editor.org/rfc/rfc3986#page-50
    static const std::regex url_regex(R"(^(?:([^:\/?#]+)://)?(?:([^:\/?#]*)(?::([0-9]+))?)?([^:?#]*)(?:\?([^#]*))?(?:#(.*))?)");
    int counter = 0;
    std::cmatch url_match_result;

    /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
    if (!std::regex_match(inurl.data(), inurl.data()+ inurl.size(), url_match_result, url_regex)) {
        return;
    }

    std::csub_match subMatch = url_match_result[1];
    ParsedURL.outScheme=std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[2];
    ParsedURL.outAuthority = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[3];
    ParsedURL.outPort = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[4];
    ParsedURL.outPath = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[5];
    auto queryStr = std::string_view(subMatch.first, subMatch.second);

    static const std::regex param_regex(R"(([^&#=]+)(?:=([^&#]*))?)");
    auto begin = std::cregex_iterator(queryStr.data(), queryStr.data() + queryStr.size(), param_regex);
    auto end = std::cregex_iterator();

    for (auto it = begin; it != end; ++it) {
        const std::cmatch& m = *it;
        std::string_view key = get_sv(m, 1, queryStr);
        std::string_view value = (m[2].matched) ? get_sv(m, 2, queryStr) : std::string_view{};
        ParsedURL.outQuery.emplace_back(key, value);
    }


    subMatch = url_match_result[6];
    ParsedURL.outFragment = std::string_view(subMatch.first, subMatch.second);
}