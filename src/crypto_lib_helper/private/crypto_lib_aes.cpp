#include "crypto_lib_aes.h"
#include <FunctionExitHelper.h>
#include <char_buffer_extension.h>
#include <string_convert.h>
#ifdef HAS_MbedTLS
#include <mbedtls/cipher.h>

#elif defined HAS_OpenSSL
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/param_build.h>
#endif

static const size_t AES_BLOCK_SIZE = 256;
typedef struct AESWorkData_t {
#ifdef HAS_MbedTLS
    mbedtls_cipher_context_t ctx{};
#elif defined HAS_OpenSSL
    EVP_CIPHER_CTX* ctx{};
#endif
}AESWorkData_t;

CommonHandlePtr_t CryptoLibAESEncryptBegin(std::span<const uint8_t> key, std::span<const uint8_t> iv)
{
    bool bRes{ true };
    FCharBuffer keyBuffer;
    FCharBuffer ivBuffer;
    AESWorkData_t* pData = new AESWorkData_t;
    auto& Data = *pData;
    FunctionExitHelper_t ptrGuard(
        [&]() {
            if (!bRes) {
                delete pData;
            }
        }
    );
    if (key.size() == 0) {
        return NullHandle;
    }
#ifdef HAS_MbedTLS
    mbedtls_cipher_init(&Data.ctx);
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);
    bRes = mbedtls_cipher_setup(&Data.ctx, info) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_setkey(&Data.ctx, key.data(), key.size() * CHAR_BIT, MBEDTLS_ENCRYPT) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_set_iv(&Data.ctx, iv.data(), iv.size()) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_set_padding_mode(&Data.ctx, MBEDTLS_PADDING_PKCS7) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_reset(&Data.ctx) == 0;
    if (!bRes) {
        return NullHandle;
    }
#elif defined HAS_OpenSSL
    Data.ctx = EVP_CIPHER_CTX_new();
    bRes = Data.ctx;
    if (!bRes) {
        return NullHandle;
    }
    FunctionExitHelper_t ctxGuard(
        [&]() {
            if (!bRes) {
                EVP_CIPHER_CTX_free(Data.ctx);
            }
        }
    );

    bRes = EVP_EncryptInit_ex(Data.ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)GetStringViewCStr(ConvertSpanToView(key), keyBuffer), (const unsigned char*)GetStringViewCStr(ConvertSpanToView(key), keyBuffer)) == 1;
    if (!bRes) {
        return NullHandle;
    }
    bRes = EVP_CIPHER_CTX_set_padding(Data.ctx, EVP_PADDING_PKCS7) == 1;
    if (!bRes) {
        return NullHandle;
    }
#endif
    return CommonHandlePtr_t(intptr_t(pData));
}

CommonHandlePtr_t CryptoLibAESDecryptBegin(std::span<const uint8_t> key, std::span<const uint8_t> iv)
{
    bool bRes{ true };
    FCharBuffer keyBuffer;
    FCharBuffer ivBuffer;
    AESWorkData_t* pData = new AESWorkData_t;
    auto& Data = *pData;
    FunctionExitHelper_t ptrGuard(
        [&]() {
            if (!bRes) {
                delete pData;
            }
        }
    );
    if (key.size() == 0) {
        return NullHandle;
    }
#ifdef HAS_MbedTLS
    mbedtls_cipher_init(&Data.ctx);
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);
    bRes = mbedtls_cipher_setup(&Data.ctx, info) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_setkey(&Data.ctx, key.data(), key.size() * CHAR_BIT, MBEDTLS_DECRYPT) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_set_iv(&Data.ctx, iv.data(), iv.size()) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_set_padding_mode(&Data.ctx, MBEDTLS_PADDING_PKCS7) == 0;
    if (!bRes) {
        return NullHandle;
    }
    bRes = mbedtls_cipher_reset(&Data.ctx) == 0;
    if (!bRes) {
        return NullHandle;
    }
#elif defined HAS_OpenSSL
    Data.ctx = EVP_CIPHER_CTX_new();
    bRes = Data.ctx;
    if (!bRes) {
        return NullHandle;
    }
    FunctionExitHelper_t ctxGuard(
        [&]() {
            if (!bRes) {
                EVP_CIPHER_CTX_free(Data.ctx);
            }
        }
    );

    bRes = EVP_DecryptInit_ex(Data.ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)GetStringViewCStr(ConvertSpanToView(key), keyBuffer), (const unsigned char*)GetStringViewCStr(ConvertSpanToView(key), keyBuffer)) == 1;
    if (!bRes) {
        return NullHandle;
    }

#endif
    return CommonHandlePtr_t(intptr_t(pData));
}

bool CryptoLibAESEncryptUpdate(CommonHandlePtr_t handle, std::span<const uint8_t> src, FCharBuffer& buf)
{
    bool bRes{ true };
    auto& Data = *(AESWorkData_t*)handle.ID;
    buf.Reverse(buf.Size() + src.size());

#ifdef HAS_MbedTLS
    size_t outLen = buf.Capacity() - buf.Size();
    bRes = mbedtls_cipher_update(&Data.ctx, src.data(), src.size(), reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen) == 0;
#elif defined HAS_OpenSSL
    int outLen = buf.Capacity() - buf.Size();
    bRes = EVP_EncryptUpdate(Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen,
        src.data(), src.size()) == 1;
#endif
    return bRes;
}

bool CryptoLibAESEncryptFinal(CommonHandlePtr_t handle, FCharBuffer& buf)
{
    bool bRes{ true };
    auto& Data = *(AESWorkData_t*)handle.ID;
   
#ifdef HAS_MbedTLS
    size_t outLen = buf.Capacity() - buf.Size();
    bRes = mbedtls_cipher_finish(&Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen)==0;
#elif defined HAS_OpenSSL
    int outLen = buf.Capacity() - buf.Size();
    bRes = EVP_EncryptFinal_ex(Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen) == 1;
#endif
    return bRes;
}

bool CryptoLibAESDecryptUpdate(CommonHandlePtr_t handle, std::span<const uint8_t> src, FCharBuffer& buf)
{
    bool bRes{ true };
    auto& Data = *(AESWorkData_t*)handle.ID;
    buf.Reverse(buf.Size() + src.size());

#ifdef HAS_MbedTLS
    size_t outLen = buf.Capacity() - buf.Size();
    bRes = mbedtls_cipher_update(&Data.ctx, src.data(), src.size(), reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen) == 0;
#elif defined HAS_OpenSSL
    int outLen = buf.Capacity() - buf.Size();
    bRes = EVP_DecryptUpdate(Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen,
        src.data(), src.size()) == 1;
#endif
    return bRes;
}

bool CryptoLibAESDecryptFinal(CommonHandlePtr_t handle, FCharBuffer& buf)
{
    bool bRes{ true };
    auto& Data = *(AESWorkData_t*)handle.ID;

#ifdef HAS_MbedTLS
    size_t outLen = buf.Capacity() - buf.Size();
    bRes = mbedtls_cipher_finish(&Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen) == 0;
#elif defined HAS_OpenSSL
    int outLen = buf.Capacity() - buf.Size();
    bRes = EVP_DecryptFinal_ex(Data.ctx, reinterpret_cast<unsigned char*> (buf.Data() + buf.Size()), &outLen) == 1;
#endif
    return bRes;
}

void CryptoLibAESRelease(CommonHandlePtr_t handle)
{
    auto& Data = *(AESWorkData_t*)handle.ID;
#ifdef HAS_MbedTLS
    mbedtls_cipher_free(&Data.ctx);
#elif defined HAS_OpenSSL
    EVP_CIPHER_CTX_free(Data.ctx);
#endif
    delete (AESWorkData_t*)handle.ID;
    handle.Reset();
    return;
}
