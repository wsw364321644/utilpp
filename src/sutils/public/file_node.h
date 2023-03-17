/**
 *  file_node.h
 */

#pragma once

#include "raw_file.h"
#include "map_file.h"
#include <vector>

class CFileNode;
typedef std::shared_ptr<CFileNode> CFileNodePtr;

class CFileNode
    : public std::enable_shared_from_this<CFileNode>
{
public:
    using std::enable_shared_from_this<CFileNode>::shared_from_this;

    CFileNode();
    ~CFileNode();
    CFileNode(const char* lpParentDir, const char* lpName, uint64_t uSize, bool bDir);
    friend bool operator== (const CFileNode & left, const CFileNode & right);

    int32_t Attached(CFileNodePtr ptrParentNode);
    void ChangeContentDir(const char* lpParentDir);

    // Getting & Setting
    CFileNodePtr	GetParent();
    uint32_t	GetSubCount();
    const char* GetName() const;
    const char* GetFullName() const;
    std::string&  GetFullNamePointer();

    uint64_t	GetSize() const;
    bool	IsDir() const;
    uint32_t	GetBlocks();
    uint32_t	GetCompletedBlocks();
    uint8_t * GetFileShaCode();

    void	SetIsDir(bool bDir);
    void	SetName(const char* lpName);
    void	SetSize(uint64_t size);
    void	SetBlocks(uint32_t uBlocks);
    void	AccCompletedBlocks();
    void	SetInvalid(bool bInvalid);
    void	SetAllInvalid(bool bInvalid);
    bool	IsInvalid();
    void	SetShaPassed(bool bPassed);
    bool	IsShaPassed();
    uint32_t GetBlockNUM() ;
    void  SetBlockNUM(uint32_t NUM) ;

    std::vector<CFileNodePtr> &	GetSubNodeList();

    static void MarkSameFile(std::vector<CFileNodePtr> & srcSubList, std::vector<CFileNodePtr> & dstSubList);
private:
    std::vector<CFileNodePtr> m_SubNodeList;
    CFileNodePtr m_ptrParentNode;
    bool m_bShaPassed;
    bool m_bInvalid;
    bool m_bDir;
    std::string m_lpName;
    std::string m_lpFullName;
    uint64_t m_uSize;
    uint32_t m_uBlocks;
    uint32_t m_uCompletedBlocks;
    std::vector<uint8_t> m_FileShaCode;
    uint32_t m_uBlockNUM;
};