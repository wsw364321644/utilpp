#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <span>
#include <string_buffer.h>

#include "crypto_lib_export_defs.h"

/**
* @param inoutlen input des buf bytes, out number of bytes written
*/
CRYPTO_LIB_HELPER_API bool crypto_lib_base64_encode(const uint8_t* src, size_t insize, uint8_t* const dst, size_t* inoutlen);
CRYPTO_LIB_HELPER_API bool crypto_lib_base64_decode(const uint8_t* src, size_t insize, uint8_t* const dst, size_t* inoutlen);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibBase64Encode(std::span<uint8_t> src, FCharBuffer& buf);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibBase64Decode(std::span<uint8_t> src, FCharBuffer& buf);