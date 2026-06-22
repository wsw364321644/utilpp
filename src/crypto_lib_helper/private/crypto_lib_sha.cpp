#include "crypto_lib_sha.h"
#include "FunctionExitHelper.h"

#ifdef HAS_MbedTLS
#include <mbedtls/psa_util.h>
#include "mbedtls/ssl.h"
#elif defined HAS_OpenSSL
#include <openssl/evp.h>
#endif
#include <assert.h>
typedef struct SHA256WorkData_t {
#ifdef HAS_MbedTLS
    psa_status_t status;
    psa_algorithm_t alg{ PSA_ALG_SHA_256 };
    psa_hash_operation_t operation =PSA_HASH_OPERATION_INIT;
#elif defined HAS_OpenSSL
    EVP_MD_CTX* ctx{};
#endif
}SHA256WorkData_t;

CommonHandlePtr_t CryptoLibSHA256Begin()
{
    bool bRes{ true };
    SHA256WorkData_t* pData = new SHA256WorkData_t;
    auto& Data = *pData;
    FunctionExitHelper_t ptrGuard(
        [&]() {
            if (!bRes) {
                delete pData;
            }
        }
    );
#ifdef HAS_MbedTLS

    Data.status = psa_crypto_init();
    if (Data.status != PSA_SUCCESS) {
        return NullHandle;
    }
    Data.status = psa_hash_setup(&Data.operation, Data.alg);
    if (Data.status != PSA_SUCCESS) {
        return NullHandle;
    }
#elif defined HAS_OpenSSL
    Data.ctx = EVP_MD_CTX_create();
    bRes = Data.ctx;
    if (!bRes) {
        return NullHandle;
    }
    FunctionExitHelper_t ctxGuard(
        [&]() {
            if (!bRes) {
                EVP_MD_CTX_destroy(Data.ctx);
            }
        }
    );
    bRes = EVP_MD_CTX_init(Data.ctx) == 1;
    if (!bRes) {
        return NullHandle;
    }
    bRes = EVP_DigestInit_ex(Data.ctx, EVP_sha256(), NULL)==1;
    if (!bRes) {
        return NullHandle;
    }

#endif
    return CommonHandlePtr_t(intptr_t(pData));
}

void CryptoLibSHA256Reset(CommonHandlePtr_t handle)
{
    auto& Data = *(SHA256WorkData_t*)handle.ID;
#ifdef HAS_MbedTLS
    Data.status = psa_hash_setup(&Data.operation, Data.alg);
    if (Data.status != PSA_SUCCESS) {
        assert(false);
    }
#elif defined HAS_OpenSSL
    EVP_MD_CTX_reset(Data.ctx);
#endif
    return;
}

bool CryptoLibSHA256Update(CommonHandlePtr_t handle, std::span<const uint8_t> src)
{
    auto& Data = *(SHA256WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    Data.status = psa_hash_update(&Data.operation, src.data(), src.size());
    bRes = Data.status == PSA_SUCCESS;
#elif defined HAS_OpenSSL
    bRes = EVP_DigestUpdate(Data.ctx, src.data(), src.size()) == 1;
#endif
    return bRes;
}

bool CryptoLibSHA256Digest(CommonHandlePtr_t handle, std::span<uint8_t> buf, uint32_t* outlen)
{
    auto& Data = *(SHA256WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    size_t lengthOfHash = 0;
    Data.status = psa_hash_finish(&Data.operation, buf.data(), buf.size(), &lengthOfHash);
    if (outlen) {
        *outlen = lengthOfHash;
    }
    if (Data.status != PSA_SUCCESS) {
        return false;
    }
    psa_hash_abort(&Data.operation);
#elif defined HAS_OpenSSL
    unsigned int md_len;
    bRes = EVP_DigestFinal_ex(Data.ctx,buf.data(), &md_len) == 1;
    if (outlen) {
        *outlen = md_len;
    }
#endif
    return bRes;
}

void CryptoLibSHA256Release(CommonHandlePtr_t handle)
{
    auto& Data = *(SHA256WorkData_t*)handle.ID;
    bool bRes{ true };
#ifdef HAS_MbedTLS
    mbedtls_psa_crypto_free();
#elif defined HAS_OpenSSL
    EVP_MD_CTX_destroy(Data.ctx);
#endif
    delete (SHA256WorkData_t*)handle.ID;
    handle.Reset();
    return;
}
