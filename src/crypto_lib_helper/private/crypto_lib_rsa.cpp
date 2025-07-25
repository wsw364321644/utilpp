#include "crypto_lib_rsa.h"
#include "FunctionExitHelper.h"
#ifdef HAS_MbedTLS
#include <mbedtls/base64.h>
#elif defined HAS_OpenSSL
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/param_build.h>
#endif
class FRSAKeyHandle : public ICommonHandle {
public:
#ifdef HAS_MbedTLS
#elif defined HAS_OpenSSL
    ~FRSAKeyHandle() {
        if (n) {
            BN_free(n);
        }
        if (e) {
            BN_free(e);
        }
    }
    bool IsValid()const override{
        return !!key;
    }
    BIGNUM* n{ NULL };
    BIGNUM* e{ NULL };
    EVP_PKEY* key{ NULL };
    EVP_PKEY_CTX* genctx{ NULL };
#endif
};
std::shared_ptr<ICommonHandle> CryptoLibRSAGetKey(const FCryptoLibBinParams& params)
{
#ifdef HAS_MbedTLS
    return nullptr;
#elif defined HAS_OpenSSL
    auto phandle = std::make_shared< FRSAKeyHandle>();
    auto& handle = *phandle;

    auto moditr=params.find("mod");
    auto mitr=params.find("m");
    auto expitr=params.find("exp");
    auto eitr=params.find("e");
    const std::span<uint8_t>* modParam{ nullptr };
    const std::span<uint8_t>* expParam{ nullptr };
    if (moditr != params.end()) {
        modParam = &moditr->second;
    }
    else if (mitr != params.end()) {
        modParam = &mitr->second;
    }
    if (expitr != params.end()) {
        expParam = &expitr->second;
    }
    else if (eitr != params.end()) {
        expParam = &eitr->second;
    }
    if (modParam && expParam) {
        handle.n = BN_bin2bn(modParam->data(), modParam->size(), NULL);
        if (!handle.n) {
            return nullptr;
        }
        handle.e = BN_bin2bn(expParam->data(), expParam->size(), NULL);
        if (!handle.e) {
            return nullptr;
        }
        OSSL_PARAM_BLD* param_bld{ NULL };
        OSSL_PARAM* params = NULL;
        param_bld = OSSL_PARAM_BLD_new();
        OSSL_PARAM_BLD_push_BN(param_bld, "n", handle.n);
        OSSL_PARAM_BLD_push_BN(param_bld, "e", handle.e);
        params = OSSL_PARAM_BLD_to_param(param_bld);
        handle.genctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
        FunctionExitHelper_t paramsHelper(
            [&]() {
            if (params)
                OSSL_PARAM_free(params);
            if (param_bld)
                OSSL_PARAM_BLD_free(param_bld);
            }
        );
        if (!handle.genctx)
            return nullptr;
        if (EVP_PKEY_fromdata_init(handle.genctx) <= 0) {
            return nullptr;
        }
        auto ires = EVP_PKEY_fromdata(handle.genctx, &handle.key, EVP_PKEY_PUBLIC_KEY, params);
        if (ires <= 0) {
            return nullptr;
        }
    }
    return  phandle;
#endif
}

bool CryptoLibRSAEncrypt(ICommonHandle* handle, std::span<uint8_t> src, FCharBuffer& buf)
{
#ifdef HAS_MbedTLS
    return false;
#elif defined HAS_OpenSSL
    auto KeyHandle =dynamic_cast<FRSAKeyHandle*>(handle);
    if (!KeyHandle || !KeyHandle->IsValid()) {
        return false;
    }
    size_t cipherlen;
    BIO* b64{ nullptr };

    EVP_PKEY_CTX* ctx = NULL;
    ENGINE* eng = NULL;
    auto pad = RSA_PKCS1_PADDING;
    ctx = EVP_PKEY_CTX_new(KeyHandle->key, eng);
    if (!ctx)
        return false;
    auto ires = EVP_PKEY_encrypt_init(ctx);
    if (ires <= 0)
        return false;
    if (EVP_PKEY_CTX_ctrl(ctx, -1, -1, EVP_PKEY_CTRL_RSA_PADDING, pad, NULL) <= 0)
        return false;
    if (EVP_PKEY_encrypt(ctx, NULL, &cipherlen, src.data(), src.size()) <= 0)
        return false;
    buf.Reverse(cipherlen);
    if (EVP_PKEY_encrypt(ctx, (unsigned char*)buf.Data(), &cipherlen, src.data(), src.size()) <= 0)
        return false;
    buf.SetLength(cipherlen);
    return true;
#endif
}
