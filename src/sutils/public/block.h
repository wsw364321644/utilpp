/**
 *  block.h
 */

#pragma once

#include "file_pool.h"
#include "file_node.h"
#include "hash.h"
#include "defs.h"

#include <array>

#define MAX_BLOCK_SIZE (256 * 1024)
class CFileNode;

enum EBlockState
{
    EBlockState_Idle,
    EBlockState_Requesting,
    EBlockState_Writing,
};

struct SRefFilePart
{
    CFileNodePtr    ptrFileNode;
    uint64_t        uFileBegin;
    uint64_t        uFileEnd;
};

class CP2PBlock
{
public:
    CP2PBlock();
    ~CP2PBlock();

    void WriteData(const void* data, size_t offset, size_t size);
    int32_t WriteBlock(CFilePoolPtr pool);
    int32_t ReadBlock(CFilePoolPtr pool);
    void  AllocBlock();
    void  FreeBlock();
    void  InsertRefFile(SRefFilePart & part);
    bool  ShaCheck();

    uint32_t GetBlockSize();
    void AccBlockSize(uint32_t size);

    uint32_t GetIndex();
    void SetIndex(uint32_t index);

    EBlockState GetBlockState();
    void SetBlockState(EBlockState state);
    void SetShaCodes(uint8_t * codes);
    uint8_t const * GetShaCodes() const;
    void SetDownloadFlag(bool flag);
    bool IsDownloadCompleted();

    std::vector<SRefFilePart> & GetRefFileList();

private:
    uint32_t    m_BlockSize;
    uint32_t	m_Index;
    uint8_t     m_SHACodes[SHA_BIN_LEN];
    EBlockState m_BlockState;

    std::vector<uint8_t> m_BlockData;
    std::vector<SRefFilePart>   m_RefFileList;
    bool m_Downloaded;
};

typedef std::shared_ptr<CP2PBlock> CP2PBlockPtr;
