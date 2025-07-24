#include "crypto_lib_base64.h"
#include <FunctionExitHelper.h>
#ifdef HAS_MbedTLS
#include <mbedtls/base64.h>
#elif defined HAS_OpenSSL
#include <openssl/evp.h>
#endif
#include <cstring>


bool crypto_lib_base64_encode(const uint8_t* src, size_t insize, uint8_t* const dst, size_t* inoutlen)
{
    if (inoutlen == nullptr || src == nullptr) {
        return false;
    }
    size_t dlen = *inoutlen;
#ifdef HAS_MbedTLS
    int res = mbedtls_base64_encode(dst, dlen, inoutlen, src, insize);
    if (res == 0) {
        return true;
    }
    if (res == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL&&(dst ==nullptr|| dlen==0)) {
        return true;
    }
    return false;
#elif defined HAS_OpenSSL
    BIO* b64{ nullptr };
    b64 = BIO_new(BIO_f_base64()); // create BIO to perform base64
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* mem = BIO_new(BIO_s_mem()); // create BIO that holds the result
    // chain base64 with mem, so writing to b64 will encode base64 and write to mem.
    BIO_push(b64, mem);
    // write data
    bool done = false;
    while (!done)
    {
        auto ires = BIO_write(b64, src, insize);
        if (ires <= 0) // if failed
        {
            if (BIO_should_retry(b64)) {
                continue;
            }
            else // encoding failed
            {
                return false;
            }
        }
        else // success!
            done = true;
    }
    BIO_flush(b64);
    // get a pointer to mem's data
    unsigned char* base64Output;
    *inoutlen = BIO_get_mem_data(mem, &base64Output);
    BIO_pop(b64);
    BIO_free_all(b64);
    FunctionExitHelper_t memHelper(
        [&]() {
            BIO_free(mem);
        }
    );
    if (dst == nullptr || dlen == 0) {
        return true;
    }
    else if (dlen >= *inoutlen) {
        memcpy(dst, base64Output, *inoutlen);
        return true;
    }
    else {
        return false;
    }
#endif
}

bool crypto_lib_base64_decode(const uint8_t* src, size_t insize, uint8_t* const dst, size_t* inoutlen)
{
    if (inoutlen == nullptr || src == nullptr) {
        return false;
    }
    size_t dlen = *inoutlen;
#ifdef HAS_MbedTLS
    int res = mbedtls_base64_decode(dst, dlen, inoutlen, src, insize);
    if (res == 0) {
        return true;
    }
    if (res == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && (dst == nullptr || dlen == 0)) {
        return true;
    }
    return false;
#elif defined HAS_OpenSSL
    BIO* b64{ nullptr };
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* source = BIO_new_mem_buf(src, insize);
    BIO_push(b64, source);
    *inoutlen = 0;
    uint8_t* out= dst;
    auto outlen = dlen;
    if (dst == nullptr || dlen == 0) {
        out = new uint8_t[insize];
        outlen = insize;
    }
    do {
         auto readlen= BIO_read(b64, out, outlen);
         if (readlen <= 0) {
             break;
         }
         *inoutlen += readlen;
    } while (true);
    
    BIO_flush(b64);
    BIO_pop(b64);
    BIO_free_all(b64);

    if (dst == nullptr || dlen == 0) {
        return true;
    }
    else if (dlen >= *inoutlen) {
        return true;
    }
    else {
        return false;
    }
#endif
}

bool CryptoLibBase64Encode(std::span<uint8_t> srcspan, FCharBuffer& buf)
{
#ifdef HAS_MbedTLS
    return false;
#elif defined HAS_OpenSSL
    BIO* b64{ nullptr };
    b64 = BIO_new(BIO_f_base64()); // create BIO to perform base64
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* mem = BIO_new(BIO_s_mem()); // create BIO that holds the result
    FunctionExitHelper_t memHelper(
        [&]() {
            BIO_free(mem);
        }
    );
    // chain base64 with mem, so writing to b64 will encode base64 and write to mem.
    BIO_push(b64, mem);
    // write data
    bool done = false;
    size_t insize = srcspan.size();
    const uint8_t* src = srcspan.data();
    while (!done)
    {
        auto ires = BIO_write(b64, src, insize);
        if (ires <= 0) // if failed
        {
            if (BIO_should_retry(b64)) {
                continue;
            }
            else // encoding failed
            {
                return false;
            }
        }
        else // success!
            done = true;
    }
    BIO_flush(b64);
    // get a pointer to mem's data
    unsigned char* base64Output;
    auto inoutlen = BIO_get_mem_data(mem, &base64Output);
    BIO_pop(b64);
    BIO_free_all(b64);

    buf.Assign((const char*)base64Output, inoutlen);
    return true;
#endif
}

 bool CryptoLibBase64Decode(std::span<uint8_t> srcspan, FCharBuffer& buf)
{
#ifdef HAS_MbedTLS
    return false;
#elif defined HAS_OpenSSL
     size_t insize = srcspan.size();
     const uint8_t* src = srcspan.data();
     BIO* b64{ nullptr };
     b64 = BIO_new(BIO_f_base64());
     BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
     BIO* source = BIO_new_mem_buf(src, insize);
     BIO_push(b64, source);
     buf.Reverse(insize);
     do {
         auto readlen = BIO_read(b64, buf.Data(), buf.Capacity());
         if (readlen <= 0) {
             break;
         }
         buf.SetLength(buf.Length() + readlen);
     } while (true);
     BIO_flush(b64);
     BIO_pop(b64);
     BIO_free_all(b64);
     return true;
#endif
}
