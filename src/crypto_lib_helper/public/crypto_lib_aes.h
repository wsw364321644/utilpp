#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <span>
#include <handle.h>
#include <CharBuffer.h>

#include "crypto_lib_types.h"
#include "crypto_lib_export_defs.h"


CRYPTO_LIB_HELPER_EXPORT CommonHandlePtr_t CryptoLibAESEncryptBegin(std::span<const uint8_t> key,std::span<const uint8_t> iv);
CRYPTO_LIB_HELPER_EXPORT CommonHandlePtr_t CryptoLibAESDecryptBegin(std::span<const uint8_t> key,std::span<const uint8_t> iv);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibAESEncryptUpdate(CommonHandlePtr_t handle, std::span<const uint8_t> src, FCharBuffer& buf);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibAESEncryptFinal(CommonHandlePtr_t handle,FCharBuffer& buf);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibAESDecryptUpdate(CommonHandlePtr_t handle, std::span<const uint8_t> src, FCharBuffer& buf);
CRYPTO_LIB_HELPER_EXPORT bool CryptoLibAESDecryptFinal(CommonHandlePtr_t handle, FCharBuffer& buf);
CRYPTO_LIB_HELPER_EXPORT void CryptoLibAESRelease(CommonHandlePtr_t handle);