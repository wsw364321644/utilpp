#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <handle.h>
#include <span>

#include "crypto_lib_export_defs.h"


CRYPTO_LIB_HELPER_EXPORT CommonHandlePtr_t CryptoLibMD5Begin();
CRYPTO_LIB_HELPER_EXPORT void CryptoLibMD5Reset(CommonHandlePtr_t handle);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibMD5Update(CommonHandlePtr_t handle, std::span<const uint8_t> src);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibMD5Digest(CommonHandlePtr_t handle, void* out);
CRYPTO_LIB_HELPER_EXPORT void CryptoLibMD5Release(CommonHandlePtr_t handle);