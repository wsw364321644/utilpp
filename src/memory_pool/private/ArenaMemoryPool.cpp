#include "ArenaMemoryPool.h"
#include <singleton.h>


void* ArenaMemoryPool::Malloc(size_t size)
{
    if (size == 0) return nullptr;
    size = (size + 7) & ~static_cast<size_t>(7); // 8字节对齐

    if (m_isUsingScratch && m_scratchOffset + size <= m_scratchSize) {
        void* ptr = m_scratchBuffer + m_scratchOffset;
        m_scratchOffset += size;
        return ptr;
    }

    m_isUsingScratch = false;
    if (!m_currentHeapChunk || m_heapOffset + size > m_heapChunkSize) {
        size_t allocSize = std::max(size, m_heapChunkSize);
        Chunk* newChunk = static_cast<Chunk*>(malloc(sizeof(Chunk) + allocSize));
        if (!newChunk) return nullptr;

        newChunk->next = m_heapHead;
        m_heapHead = newChunk;
        m_currentHeapChunk = newChunk;
        m_heapOffset = 0;
    }

    void* ptr = reinterpret_cast<char*>(m_currentHeapChunk + 1) + m_heapOffset;
    m_heapOffset += size;
    return ptr;
}

void ArenaMemoryPool::Clear()
{
    Chunk* current = m_heapHead;
    while (current) {
        Chunk* prev = current->next;
        free(current);
        current = prev;
    }
    m_heapHead = nullptr;
    m_currentHeapChunk = nullptr;
    m_heapOffset = 0;
    m_scratchOffset = 0;
    m_isUsingScratch = true;
}

ArenaMemoryPool& GetThreadArenaMemoryPool()
{
    thread_local char scratch[32 * 1024];;
    thread_local TClassSingletonHelper<ArenaMemoryPool> helper;
    return *helper.GetClassSingleton(scratch, sizeof(scratch));
}
