/**
 *  file_processer.h
 */

#pragma once

#include "file_pool.h"
#include <map>
#include <mutex>


class CFileProcesser
{
public:
    void Init();
    void Destroy();
    void CreatePool(uint32_t content, uint32_t type);
    void DeletePool(uint32_t content, uint32_t type);
    CFilePoolPtr GetFilePool(uint32_t content, uint32_t type);

    CFileProcesser();
    ~CFileProcesser();

private:
    std::mutex m_FilePoolsLock;
    std::map<uint64_t, CFilePoolPtr> m_FilePools;
};