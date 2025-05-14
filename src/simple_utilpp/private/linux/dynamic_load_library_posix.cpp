#include "dynamic_load_library.h"
#include "simple_os_defs.h"
typedef struct LibraryInfoPosix_t {
    void* lib_handle;
}LibraryInfoPosix_t;

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