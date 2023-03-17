/**
 *  file_pool.h
 */

#pragma once

#include "raw_file.h"

#include <list>
#include <unordered_map>
#include <mutex>
#define MAX_FILE_COUNT  64
class CFilePool
{
public:
    CFilePool();
    ~CFilePool();

    CRawFilePtr CreateFileHandle(const std::string &strFileName, uint64_t uExceptSize);
    CRawFilePtr GetFileHandle(const std::string &strFileName);
    void CloseAll();

private:
    void AddFileHandle(CRawFilePtr filePtr);
    volatile bool m_bStoped;
    std::list<std::string> m_KeyValues; // sorted by add time
    std::unordered_map<std::string, CRawFilePtr> m_FileHandles;
    std::mutex m_FileHandesLock;
};

typedef std::shared_ptr<CFilePool> CFilePoolPtr;