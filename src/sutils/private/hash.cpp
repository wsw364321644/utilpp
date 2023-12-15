/**
 *  hash.cpp
 */

#include "hash.h"
#include <string>
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



uint32_t simple_hash(const char* szName) {
	uint32_t hash = 0;
	uint32_t i = 0;
	for (i; i < strlen(szName); i++) {
		hash <<= 1;
		hash += szName[i];
	}
	return hash;
}