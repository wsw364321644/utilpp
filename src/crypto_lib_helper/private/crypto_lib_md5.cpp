#include "crypto_lib_md5.h"
#include "FunctionExitHelper.h"
#ifdef HAS_MbedTLS
#include <mbedtls/md5.h>
#elif defined HAS_OpenSSL
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/param_build.h>
#endif

typedef struct MD5WorkData_t {
#ifdef HAS_MbedTLS
    mbedtls_md5_context MD5ctx{};
#elif defined HAS_OpenSSL
    EVP_MD_CTX* MD5ctx{};
#endif
};

CommonHandlePtr_t CryptoLibMD5Begin()
{
    bool bRes{ true };
    MD5WorkData_t* pData = new MD5WorkData_t;
    auto& Data = *pData;
    FunctionExitHelper_t ptrGuard(
        [&]() {
            if (!bRes) {
                delete pData;
            }
        }
    );
#ifdef HAS_MbedTLS
    mbedtls_md5_init(&pData->MD5ctx);
#elif defined HAS_OpenSSL
    Data.MD5ctx = EVP_MD_CTX_create();
    FunctionExitHelper_t ctxGuard(
        [&]() {
            if (!bRes) {
                EVP_MD_CTX_destroy(Data.MD5ctx);
            }
        }
    );
    bRes = Data.MD5ctx;
    if (!bRes) {
        return NullHandle;
    }
    bRes=EVP_MD_CTX_init(Data.MD5ctx) == 1;
    if (!bRes) {
        return NullHandle;
    }
    bRes = EVP_DigestInit_ex(Data.MD5ctx, EVP_md5(), NULL) == 1;
    if (!bRes) {
        return NullHandle;
    }

#endif
    return CommonHandlePtr_t(intptr_t(pData));
}

void CryptoLibMD5Reset(CommonHandlePtr_t handle)
{
    auto& Data = *(MD5WorkData_t*)handle.ID;
#ifdef HAS_MbedTLS
    mbedtls_md5_init(&Data.MD5ctx);
#elif defined HAS_OpenSSL
    EVP_MD_CTX_reset(Data.MD5ctx);
#endif
    return;
}

bool CryptoLibMD5Update(CommonHandlePtr_t handle, std::span<const uint8_t> src)
{
    auto& Data = *(MD5WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    bRes = mbedtls_md5_update(&Data.MD5ctx, src.data(), src.size()) == 0;
#elif defined HAS_OpenSSL
    bRes = EVP_DigestUpdate(Data.MD5ctx, src.data(), src.size()) == 1;
#endif
    return bRes;
}

bool CryptoLibMD5Digest(CommonHandlePtr_t handle, void* out)
{
    auto& Data = *(MD5WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    bRes = mbedtls_md5_finish(&Data.MD5ctx, (uint8_t*)out) == 0;
#elif defined HAS_OpenSSL
    unsigned int md_len;
    bRes = EVP_DigestFinal_ex(Data.MD5ctx, (uint8_t*)out, &md_len) == 1;
#endif
    return bRes;
}

void CryptoLibMD5Release(CommonHandlePtr_t handle)
{
    auto& Data = *(MD5WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    mbedtls_md5_free(&Data.MD5ctx);
#elif defined HAS_OpenSSL
    EVP_MD_CTX_destroy(Data.MD5ctx);
#endif
    delete (MD5WorkData_t*)handle.ID;
    handle.Reset();
    return;
}
