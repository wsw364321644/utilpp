/**
 *  file_pool.cpp
 */

#include "file_pool.h"
#include "logger.h"
#include "defs.h"

CFilePool::CFilePool()
    : m_bStoped(false)
{
}

CFilePool::~CFilePool()
{
}

CRawFilePtr CFilePool::CreateFileHandle(const std::string &strFileName, uint64_t uExceptSize)
{
    std::unique_lock<std::mutex> lock(m_FileHandesLock);
    if (!m_bStoped)
    {
        // Remove the old file handle
        auto file_it = m_FileHandles.find(strFileName);
        if (file_it != m_FileHandles.end())
        {
            for (auto key_it = m_KeyValues.begin(); key_it != m_KeyValues.end();)
            {
                if (*key_it == strFileName)
                {
                    key_it = m_KeyValues.erase(key_it);
                }
                else
                {
                    ++key_it;
                }
            }
            m_FileHandles.erase(file_it);
        }

        // New file handle
        CRawFilePtr filePtr(new CRawFile());
        if (ERR_SUCCESS == filePtr->Open(strFileName.c_str(), uExceptSize, SK_OPEN_ALWAYS))
        {
            AddFileHandle(filePtr);
        }
        else
        {
            filePtr.reset();
            LOG_ERROR("Can't create the file: {}", strFileName);
        }

        return filePtr;
    }

    return CRawFilePtr();
}

CRawFilePtr CFilePool::GetFileHandle(const std::string &strFileName)
{
    std::unique_lock<std::mutex> lock(m_FileHandesLock);

    if (!m_bStoped)
    {
        auto it = m_FileHandles.find(strFileName);
        if (it != m_FileHandles.end())
        {
            return (*it).second;
        }
        else
        {
            CRawFilePtr filePtr(new CRawFile());
            if (ERR_SUCCESS == filePtr->Open(strFileName.c_str(), 0, SK_OPEN_EXISTING))
            {
                AddFileHandle(filePtr);
            }
            else
            {
                filePtr.reset();
            }

            return filePtr;
        }
    }

    return CRawFilePtr();
}

void CFilePool::AddFileHandle(CRawFilePtr filePtr)
{
    if (m_FileHandles.size() >= MAX_FILE_COUNT)
    {
        assert(m_FileHandles.size() == m_KeyValues.size());
        auto strKey = m_KeyValues.front();
        if (m_FileHandles[strKey].get() != NULL)
        {
            m_FileHandles[strKey]->Close();
        }
        auto erase_num = m_FileHandles.erase(strKey);
        m_KeyValues.pop_front();
    }

    std::string fileName(filePtr->GetFileName());
    m_KeyValues.push_back(fileName);
    m_FileHandles[fileName] = filePtr;
}

void CFilePool::CloseAll()
{
    std::unique_lock<std::mutex> lock(m_FileHandesLock);

    m_bStoped = true;
    m_KeyValues.clear();
    for (auto it = m_FileHandles.begin(); it != m_FileHandles.end(); ++it)
    {
        (*it).second->Close();
    }
    m_FileHandles.clear();
}