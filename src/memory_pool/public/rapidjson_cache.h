#pragma once
#include "ArenaMemoryPool.h"
#include <rapidjson/document.h>
#include <string>
namespace utilpp {
    inline rapidjson::GenericStringRef<std::string_view::value_type> GetStringRef(std::string_view view) {
        return rapidjson::StringRef(view.data(), view.size());
    }

    class RapidJsonAllocatorAdapter {
    public:
        // RapidJSON 要求的静态常量
        static const bool kNeedFree = true;

        explicit RapidJsonAllocatorAdapter() :m_pool(GetThreadArenaMemoryPool()){}

        void* Malloc(size_t size) { return m_pool.Malloc(size); }
        void* Realloc(void* ptr, size_t oldSize, size_t newSize) {
            return m_pool.Realloc(ptr, oldSize, newSize);
        }
        static void Free(void* ptr) { ArenaMemoryPool::Free(ptr); }

    private:
        ArenaMemoryPool& m_pool;
    };

    typedef rapidjson::GenericDocument<rapidjson::UTF8<>, RapidJsonAllocatorAdapter> DocumentType;
}

