#pragma once
#include <string>
#include <regex>
inline void ParseUrl(const std::string& inurl, std::string* outScheme = nullptr, std::string* outAuthority = nullptr, std::string* outPort = nullptr, std::string* outPath = nullptr, std::string* outQuery = nullptr, std::string* outFragment = nullptr) {
    //https://www.rfc-editor.org/rfc/rfc3986#page-50
    std::regex url_regex(
        R"(^(([^:\/?#]+):\/\/)?(([^:\/?#]*)(:([0-9]+))?)?([^:?#]*)(\?([^#]*))?(#(.*))?)",
        std::regex::extended
    );
    std::smatch url_match_result;
    int counter = 0;
    /*url.assign( R"###(localhost.com/path\?hue\=br\#cool)###");*/
    if (!std::regex_match(inurl, url_match_result, url_regex)) {
        return;
    }
    if (outScheme) {
        *outScheme = url_match_result[2];
    }
    if (outAuthority) {
        *outAuthority = url_match_result[4];
    }
    if (outPort) {
        *outPort = url_match_result[6];
    }
    if (outPath) {
        *outPath = url_match_result[7];
    }
    if (outQuery) {
        *outQuery = url_match_result[9];
    }
    if (outFragment) {
        *outFragment = url_match_result[11];
    }
}