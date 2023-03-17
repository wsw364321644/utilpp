/**
 *  block.cpp
 */

#include "block.h"
#include "file_node.h"
#include "logger.h"


CP2PBlock::CP2PBlock()
    : m_BlockSize(0),
    m_BlockState(EBlockState_Idle),
    m_Index(0),
    m_Downloaded(false)
{
}

CP2PBlock::~CP2PBlock()
{
}

void CP2PBlock::WriteData(const void* data, size_t offset, size_t size)
{
    assert(data != NULL);
    assert(offset + size <= MAX_BLOCK_SIZE);
    memcpy(&m_BlockData[offset], data, size);
}

int32_t CP2PBlock::WriteBlock(CFilePoolPtr filePool)
{
    if (!filePool || m_BlockData.empty()) {
        return ERR_FAILED;
    }

    uint32_t uPktBufBegin = 0;
    int32_t iRet = ERR_SUCCESS;

    for (auto itFile = m_RefFileList.begin(); itFile != m_RefFileList.end(); ++itFile) {
        SRefFilePart& FilePart = (*itFile);
        LOG_INFO("WriteBlock u8 name{}", FilePart.ptrFileNode->GetFullName());
        if (!FilePart.ptrFileNode || FilePart.uFileEnd <= FilePart.uFileBegin)
        {
            LOG_WARNING("Error file part.");
            iRet = ERR_FAILED;
            break;
        }

        if (!FilePart.ptrFileNode->IsInvalid()) {
            if (iRet != ERR_SUCCESS) {
                LOG_ERROR("Can't open the file for block writing: {}", FilePart.ptrFileNode->GetFullName());
                return ERR_FILE;
            }

            // Write the buff
            uint32_t WrittingSize = static_cast<uint32_t>(FilePart.uFileEnd - FilePart.uFileBegin);
            assert(WrittingSize + uPktBufBegin <= MAX_BLOCK_SIZE); 
            CRawFilePtr fileHandle = filePool->GetFileHandle(FilePart.ptrFileNode->GetFullName()); 
            // Does't existed? Create it 
            if (!fileHandle) { 
                fileHandle = filePool->CreateFileHandle(FilePart.ptrFileNode->GetFullName(), FilePart.ptrFileNode->GetSize()); 
            }

            if (fileHandle
                && ERR_SUCCESS == fileHandle->Seek(FilePart.uFileBegin)
                && ERR_SUCCESS == fileHandle->Write(&m_BlockData[uPktBufBegin], WrittingSize)) { 
                uPktBufBegin += WrittingSize; 
            } else { 
                LOG_WARNING("Failed to seek or write while saving block"); 
                iRet = ERR_FAILED; 
                break;
            }
        } else {
            uPktBufBegin += static_cast<uint32_t>(FilePart.uFileEnd - FilePart.uFileBegin);
        }
    }

    return iRet;
}

int32_t CP2PBlock::ReadBlock(CFilePoolPtr filePool)
{
    if (!filePool || !m_BlockData.empty()) {
        return ERR_FAILED;
    }

    uint32_t uPktBufBegin = 0;

    int32_t iRet = ERR_SUCCESS;
    for (auto itFile = m_RefFileList.begin(); itFile != m_RefFileList.end(); ++itFile) {
        SRefFilePart & FilePart = (*itFile);
        if (!FilePart.ptrFileNode || FilePart.uFileEnd <= FilePart.uFileBegin) {
            LOG_WARNING("Error file part.");
            iRet = ERR_FAILED;
            break;
        }

        if (!FilePart.ptrFileNode->IsInvalid()) {
            // Read the buff
            uint32_t ReadingSize = static_cast<uint32_t>(FilePart.uFileEnd - FilePart.uFileBegin);
            assert(ReadingSize + uPktBufBegin <= MAX_BLOCK_SIZE); 
            CRawFilePtr fileHandle = filePool->GetFileHandle(FilePart.ptrFileNode->GetFullName()); 
            if (fileHandle 
                && ERR_SUCCESS == fileHandle->Seek(FilePart.uFileBegin) 
                && ERR_SUCCESS == fileHandle->Read(&m_BlockData[uPktBufBegin], ReadingSize)) { 
                uPktBufBegin += ReadingSize; 
            } else { 
                iRet = ERR_FAILED; 
                LOG_WARNING("Failed to read block {}, file {}, [{}]-[{}]",
                    m_Index, FilePart.ptrFileNode->GetFullName(), FilePart.uFileBegin, FilePart.uFileEnd);
                break; 
            }
        }
    }

    return iRet;
}

void CP2PBlock::AllocBlock()
{
    if (m_BlockData.empty())
        m_BlockData.assign(MAX_BLOCK_SIZE, 0);
}

void CP2PBlock::FreeBlock()
{
    if (m_BlockData.empty())
        return;

    m_BlockData.resize(0);
    m_BlockData.shrink_to_fit();
}

bool  CP2PBlock::ShaCheck()
{
    if (m_BlockData.empty()) {
        LOG_WARNING("Can't check the sha, open and read the data first.");
        return false;
    }

    CSHA sha;
    sha.Reset();
    sha.Add(&m_BlockData[0], MAX_BLOCK_SIZE);

    uint8_t codes[SHA_BIN_LEN];
    sha.GetHash(&codes[0]);
    return (0 == memcmp(&m_SHACodes[0], &codes[0], SHA_BIN_LEN));
}

EBlockState CP2PBlock::GetBlockState()
{
    return m_BlockState;
}

void CP2PBlock::SetBlockState(EBlockState BlockState)
{
    m_BlockState = BlockState;
}

void CP2PBlock::SetShaCodes(uint8_t * pShaCodes)
{
    if (pShaCodes) {
        memcpy(m_SHACodes, pShaCodes, SHA_BIN_LEN);
    }
}

uint8_t const * CP2PBlock::GetShaCodes() const
{
    return m_SHACodes;
}

void CP2PBlock::SetDownloadFlag(bool flag)
{
    m_Downloaded = flag;
}

bool CP2PBlock::IsDownloadCompleted()
{
    return m_Downloaded;
}

uint32_t CP2PBlock::GetBlockSize()
{
    return m_BlockSize;
}

void CP2PBlock::AccBlockSize(uint32_t uBlockSize)
{
    m_BlockSize += uBlockSize;
}

uint32_t CP2PBlock::GetIndex()
{
    return m_Index;
}

void CP2PBlock::SetIndex(uint32_t index)
{
    m_Index = index;
}
std::vector<SRefFilePart> & CP2PBlock::GetRefFileList()
{
    return m_RefFileList;
}

void CP2PBlock::InsertRefFile(SRefFilePart & FilePart)
{
    m_RefFileList.push_back(FilePart);
}
