#pragma once
#include <string>
#include <regex>
typedef struct ParsedURL_t
{
    std::string_view outScheme;
    std::string_view outAuthority;
    std::string_view outPort;
    std::string_view outPath;
    std::string_view outQuery;
    std::string_view outFragment;
}ParsedURL_t;
inline void ParseUrl(std::string_view inurl, ParsedURL_t& ParsedURL) {
    //https://www.rfc-editor.org/rfc/rfc3986#page-50
    std::regex url_regex(
        R"(^(([^:\/?#]+)://)?(([^:\/?#]*)(:([0-9]+))?)?([^:?#]*)(\?([^#]*))?(#(.*))?)",
        std::regex::extended
    );
    int counter = 0;
    std::cmatch url_match_result;

    /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
    if (!std::regex_match(inurl.data(), inurl.data()+ inurl.size(), url_match_result, url_regex)) {
        return;
    }

    std::csub_match subMatch = url_match_result[2];
    ParsedURL.outScheme=std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[4];
    ParsedURL.outAuthority = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[6];
    ParsedURL.outPort = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[7];
    ParsedURL.outPath = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[9];
    ParsedURL.outQuery = std::string_view(subMatch.first, subMatch.second);

    subMatch = url_match_result[11];
    ParsedURL.outFragment = std::string_view(subMatch.first, subMatch.second);
}