#pragma once
#include <string>
#include <regex>
typedef struct ParsedURL_t
{
    std::string* outScheme{ nullptr };
    std::string* outAuthority{ nullptr };
    std::string* outPort{ nullptr };
    std::string* outPath{ nullptr };
    std::string* outQuery { nullptr };
    std::string* outFragment  { nullptr };
}ParsedURL_t;
inline void ParseUrl(const std::string& inurl, ParsedURL_t* pURL) {
    //https://www.rfc-editor.org/rfc/rfc3986#page-50
    std::regex url_regex(
        R"(^(([^:\/?#]+)://)?(([^:\/?#]*)(:([0-9]+))?)?([^:?#]*)(\?([^#]*))?(#(.*))?)",
        std::regex::extended
    );
    std::smatch url_match_result;
    int counter = 0;
    /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
    if (!std::regex_match(inurl, url_match_result, url_regex)) {
        return;
    }
    if (pURL->outScheme) {
        *pURL->outScheme = url_match_result[2];
    }
    if (pURL->outAuthority) {
        *pURL->outAuthority = url_match_result[4];
    }
    if (pURL->outPort) {
        *pURL->outPort = url_match_result[6];
    }
    if (pURL->outPath) {
        *pURL->outPath = url_match_result[7];
    }
    if (pURL->outQuery) {
        *pURL->outQuery = url_match_result[9];
    }
    if (pURL->outFragment) {
        *pURL->outFragment = url_match_result[11];
    }
}