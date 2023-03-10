/*
 * hash.h
 */

#pragma once

#include "sha1.h"
#include <stddef.h>

namespace sonkwo
{
#define SHA_BIN_LEN 20
    class CSHA
    {
    public:
        CSHA();
        ~CSHA();

        void Reset();
        void Add(const void* pData, size_t nLength);
        void GetHash(unsigned char * pHash);

    private:
        SHA1_CTX ctx_;
    };

}