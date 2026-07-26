#pragma once
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "memory_pool_export.h"

class MEMORY_POOL_EXPORT ArenaMemoryPool {
public:
    explicit ArenaMemoryPool(void* scratchBuffer, size_t scratchSize, size_t heapChunkSize = 65536)
        : m_heapChunkSize(heapChunkSize)
        , m_heapHead(nullptr), m_currentHeapChunk(nullptr), m_heapOffset(0)
        , m_scratchBuffer(static_cast<char*>(scratchBuffer))
        , m_scratchSize(scratchSize), m_scratchOffset(0)
        , m_isUsingScratch(true) {}

    ~ArenaMemoryPool() { Clear(); }

    ArenaMemoryPool(const ArenaMemoryPool&) = delete;
    ArenaMemoryPool& operator=(const ArenaMemoryPool&) = delete;

    void* Malloc(size_t size);
    void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) {
        if (originalPtr == nullptr) return Malloc(newSize);
        if (newSize == 0) { Free(originalPtr); return nullptr; }
        void* newPtr = Malloc(newSize);
        if (newPtr && originalSize > 0) {
            std::memcpy(newPtr, originalPtr, std::min(originalSize, newSize));
        }
        return newPtr;
    }

    static void Free(void* ptr) { (void)ptr; }

    void Clear();

private:
    struct Chunk { Chunk* next; };
    size_t m_heapChunkSize;
    Chunk* m_heapHead;
    Chunk* m_currentHeapChunk;
    size_t m_heapOffset;
    char* m_scratchBuffer;
    size_t m_scratchSize;
    size_t m_scratchOffset;
    bool m_isUsingScratch;
};

MEMORY_POOL_EXPORT ArenaMemoryPool& GetThreadArenaMemoryPool();