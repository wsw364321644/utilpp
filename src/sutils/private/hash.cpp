/**
 *  hash.cpp
 */

#include "hash.h"

CSHA::CSHA()
{
    SHA1Init(&ctx_);
}
CSHA::~CSHA()
{
}

void CSHA::Reset()
{
    SHA1Init(&ctx_);
}

void CSHA::Add(const void *pData, size_t nLength)
{
    SHA1Update(&ctx_, (unsigned char *)pData, nLength);
}

void CSHA::GetHash(unsigned char *pHash)
{
    SHA1Final(pHash, &ctx_);
}
