/**
 *  file_node.cpp
 */

#include "file_node.h"
#include "logger.h"
#include "hash.h"
#include <filesystem>

CFileNode::CFileNode()
    : m_bDir(false), m_uSize(0),
      m_uBlocks(0), m_uCompletedBlocks(0),
      m_bShaPassed(false), m_bInvalid(false), m_uBlockNUM(0)
{
    m_FileShaCode.assign(SHA_BIN_LEN, 0);
}

CFileNode::CFileNode(const char *lpParentDir, const char *lpName, uint64_t uSize, bool bDir)
    : m_bDir(bDir), m_uSize(uSize),
      m_uBlocks(0), m_uCompletedBlocks(0), /*m_bMapImpl(false), */
      m_bShaPassed(false), m_bInvalid(false), m_uBlockNUM(0)
{
    m_lpName = lpName;

    m_lpFullName.assign((char *)(std::filesystem::path((char8_t *)lpParentDir) / std::filesystem::path((char8_t *)lpName)).u8string().c_str());
    m_FileShaCode.assign(SHA_BIN_LEN, 0);
}

CFileNode::~CFileNode()
{
}

bool operator==(const CFileNode &left, const CFileNode &right)
{
    return (left.IsDir() == right.IsDir() && 0 ==
#ifdef WIN32
                                                 _stricmp(left.GetName(), right.GetName()));
#else
                                                 strcasecmp(left.GetName(), right.GetName()));
#endif
}

int32_t CFileNode::Attached(CFileNodePtr ptrParentNode)
{
    if (!ptrParentNode || !ptrParentNode->m_bDir)
    {
        LOG_WARNING("Wrong parent node");
        return ERR_ARGUMENT;
    }

    m_ptrParentNode = ptrParentNode;
    m_ptrParentNode->m_SubNodeList.push_back(shared_from_this());

    return ERR_SUCCESS;
}

void CFileNode::ChangeContentDir(const char *lpParentDir)
{
    m_lpFullName = (const char *)(std::filesystem::path((const char8_t *)lpParentDir) / (char8_t *)m_lpName.c_str()).lexically_normal().u8string().c_str();
    if (m_bDir)
    {
        for (std::vector<CFileNodePtr>::iterator it = m_SubNodeList.begin(); it != m_SubNodeList.end(); ++it)
        {
            (*it)->ChangeContentDir(m_lpFullName.c_str());
        }
    }
}

// O(n^2)
// We can make it more efficient algorithm ... hash_map instead of vector
void CFileNode::MarkSameFile(std::vector<CFileNodePtr> &srcSubList, std::vector<CFileNodePtr> &dstSubList)
{
    for (std::vector<CFileNodePtr>::iterator src_it = srcSubList.begin(); src_it != srcSubList.end(); ++src_it)
    {
        for (std::vector<CFileNodePtr>::iterator dst_it = dstSubList.begin(); dst_it != dstSubList.end(); ++dst_it)
        {
            if (*(*src_it) == *(*dst_it))
            {
                if (!(*src_it)->IsDir())
                {
                    (*src_it)->SetInvalid(true);
                }

                MarkSameFile((*src_it)->GetSubNodeList(), (*dst_it)->GetSubNodeList());
            }
        }
    }
}

CFileNodePtr CFileNode::GetParent()
{
    return m_ptrParentNode;
}

uint32_t CFileNode::GetSubCount()
{
    return (uint32_t)m_SubNodeList.size();
}

const char *CFileNode::GetName() const
{
    return m_lpName.c_str();
}

uint64_t CFileNode::GetSize() const
{
    return m_uSize;
}

bool CFileNode::IsDir() const
{
    return m_bDir;
}

std::vector<CFileNodePtr> &CFileNode::GetSubNodeList()
{
    return m_SubNodeList;
}

const char *CFileNode::GetFullName() const
{
    return m_lpFullName.c_str();
}

std::string &CFileNode::GetFullNamePointer()
{
    return m_lpFullName;
}

uint32_t CFileNode::GetBlocks()
{
    return m_uBlocks;
}

uint32_t CFileNode::GetCompletedBlocks()
{
    return m_uCompletedBlocks;
}

uint8_t *CFileNode::GetFileShaCode()
{
    return &m_FileShaCode[0];
}

void CFileNode::SetIsDir(bool bDir)
{
    m_bDir = bDir;
}

void CFileNode::SetName(const char *lpName)
{
    if (lpName != NULL)
    {
        m_lpName = lpName;
    }
}

void CFileNode::SetSize(uint64_t size)
{
    m_uSize = size;
}

void CFileNode::SetBlocks(uint32_t uBlocks)
{
    m_uBlocks = uBlocks;
}

void CFileNode::AccCompletedBlocks()
{
    m_uCompletedBlocks++;
}
void CFileNode::SetInvalid(bool bInvalid)
{
    m_bInvalid = bInvalid;
}

void CFileNode::SetAllInvalid(bool bInvalid)
{
    m_bInvalid = bInvalid;

    for (std::vector<CFileNodePtr>::iterator it = m_SubNodeList.begin(); it != m_SubNodeList.end(); ++it)
    {
        (*it)->SetAllInvalid(bInvalid);
    }
}

bool CFileNode::IsInvalid()
{
    return m_bInvalid;
}

void CFileNode::SetShaPassed(bool bPassed)
{
    m_bShaPassed = bPassed;
}
uint32_t CFileNode::GetBlockNUM()
{
    return m_uBlockNUM;
}

void CFileNode::SetBlockNUM(uint32_t NUM)
{
    if (m_uBlockNUM == 0)
    {
        m_uBlockNUM = NUM;
    }
}
bool CFileNode::IsShaPassed()
{
    return m_bShaPassed;
}