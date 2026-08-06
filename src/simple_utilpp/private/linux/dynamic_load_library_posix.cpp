#include "dynamic_load_library.h"
#include "simple_os_defs.h"
typedef struct LibraryInfoPosix_t {
    void* lib_handle;
}LibraryInfoPosix_t;

namespace utilpp {
    void* simple_dlopen(std::filesystem::path& libPath) {
        return simple_dlopen(libPath.c_str());
    }
}

void* simple_dlopen(const char* lib_name)
{
    auto lib_handle = ::dlopen(lib_name, RTLD_LAZY);
    if (lib_handle == nullptr)
    {
        return nullptr;
    }
    auto out = new LibraryInfoPosix_t();
    out->lib_handle = lib_handle;
    return out;
}

void* simple_dlopen_exist(const char* lib_name) {
    auto lib_handle = ::dlopen(lib_name, RTLD_NOLOAD);
    if (lib_handle == nullptr)
    {
        return nullptr;
    }
    auto out = new LibraryInfoPosix_t();
    out->lib_handle = lib_handle;
    return out;
}
void* simple_dlsym(void* handle, const char* func_name)
{
    if (!handle) {
        return NULL;
    }
    auto pLibraryInfo = reinterpret_cast<LibraryInfoPosix_t*>(handle);
    void* func_ptr = ::dlsym(pLibraryInfo->lib_handle, func_name);
    if (func_ptr == nullptr)
    {
        return nullptr;
    }
    return func_ptr;
}

bool simple_dlclose(void* handle)
{
    if (!handle) {
        return NULL;
    }
    auto pLibraryInfo = reinterpret_cast<LibraryInfoPosix_t*>(handle);
    const int rc = ::dlclose(pLibraryInfo->lib_handle);
    if (rc == 0)
    {
        return true;
    }
    return false;
}

void* simple_get_offset_addr(void* handle, uint64_t offset)
{
    auto pLibraryInfo = reinterpret_cast<LibraryInfoWin_t*>(handle);

    struct link_map *lm = NULL;
    if (::dlinfo(pLibraryInfo->lib_handle, RTLD_DI_LINKMAP, &lm) == 0 && lm != NULL) {
        return (void*)((uintptr_t)lm->l_addr+(uintptr_t)offset);
    }
    return nullptr;
}

uint32_t simple_get_module_size(void* handle)
{
    auto pLibraryInfo = reinterpret_cast<LibraryInfoWin_t*>(handle);

    struct link_map *lm = NULL;
    if (::dlinfo(pLibraryInfo->lib_handle, RTLD_DI_LINKMAP, &lm) != 0 || lm == NULL) {
        return 0;
    }

        FILE *f = fopen(lm->l_name, "rb");
    if (!f) return 0;

    ElfW(Ehdr) ehdr;
    if (fread(&ehdr, sizeof(ehdr), 1, f) != 1 || 
        memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fclose(f);
        return 0;
    }

    // Step 3: 遍历 PT_LOAD 段，取最大虚拟地址范围
    size_t max_end = 0;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        ElfW(Phdr) phdr;
        fseek(f, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
        if (fread(&phdr, sizeof(phdr), 1, f) != 1) continue;

        if (phdr.p_type == PT_LOAD) {
            size_t seg_end = phdr.p_vaddr + phdr.p_memsz;
            if (seg_end > max_end) max_end = seg_end;
        }
    }
    fclose(f);
    return max_end;
}
