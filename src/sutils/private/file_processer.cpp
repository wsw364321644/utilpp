/**
 *  file_processer.cpp
 */

#include "file_processer.h"
#include "logger_header.h"
#include "defs.h"
inline uint64_t MakeTaskKey(uint32_t content, uint32_t type)
{
    uint64_t key = content;
    return ((key << 32) | type);
}

CFileProcesser::CFileProcesser()
{
}

CFileProcesser::~CFileProcesser()
{
}

void CFileProcesser::Init()
{
}

void CFileProcesser::Destroy()
{
    std::unique_lock<std::mutex> lock(m_FilePoolsLock);
    for (auto pool : m_FilePools)
    {
        pool.second->CloseAll();
    }

    m_FilePools.clear();
}

void CFileProcesser::CreatePool(uint32_t content, uint32_t type)
{
    auto key = MakeTaskKey(content, type);
    std::unique_lock<std::mutex> lock(m_FilePoolsLock);

    if (m_FilePools.find(key) != m_FilePools.end())
    {
        LOG_WARNING("File pool for content({}:{}) already existed", content, type);
    }
    else
    {
        m_FilePools[key] = CFilePoolPtr(new CFilePool());
    }
}

void CFileProcesser::DeletePool(uint32_t content, uint32_t type)
{
    auto key = MakeTaskKey(content, type);
    std::unique_lock<std::mutex> lock(m_FilePoolsLock);

    auto it = m_FilePools.find(key);
    if (it != m_FilePools.end())
    {
        (*it).second->CloseAll();
        m_FilePools.erase(it);
    }
}

CFilePoolPtr CFileProcesser::GetFilePool(uint32_t content, uint32_t type)
{
    auto key = MakeTaskKey(content, type);
    std::unique_lock<std::mutex> lock(m_FilePoolsLock);

    if (m_FilePools.find(key) == m_FilePools.end())
    {
        LOG_WARNING("Can't get the file pool of content({}, {})", content, type);
        return CFilePoolPtr();
    }

    return m_FilePools[key];
}