#pragma once
#include <handle.h>
#include <span>

#include "crypto_lib_export_defs.h"

constexpr int MAX_SHA512_BYTE_SIZE=64;

CRYPTO_LIB_HELPER_EXPORT CommonHandlePtr_t CryptoLibSHA256Begin();
CRYPTO_LIB_HELPER_EXPORT void CryptoLibSHA256Reset(CommonHandlePtr_t handle);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibSHA256Update(CommonHandlePtr_t handle, std::span<const uint8_t> src);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibSHA256Digest(CommonHandlePtr_t handle, std::span<uint8_t>,uint32_t* outlen=nullptr);
CRYPTO_LIB_HELPER_EXPORT void CryptoLibSHA256Release(CommonHandlePtr_t handle);