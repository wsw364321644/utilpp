#pragma once
#include <span>
#include <std_ext.h>

typedef std::unordered_map<std::string, std::span<uint8_t>, string_hash> FCryptoLibBinParams;
enum class ECryptoParamType
{
    BINARY,
    HEX,
    BASE64,
};

enum class ECryptoType
{
    RSA,
};