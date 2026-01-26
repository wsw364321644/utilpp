#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <handle.h>
#include <string>
#include <CharBuffer.h>

#include "crypto_lib_types.h"
#include "crypto_lib_export_defs.h"


CRYPTO_LIB_HELPER_EXPORT std::shared_ptr<ICommonHandle> CryptoLibRSAGetKey(const FCryptoLibBinParams& params);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibRSAEncrypt(ICommonHandle* handle, std::span<uint8_t> src, FCharBuffer& buf);