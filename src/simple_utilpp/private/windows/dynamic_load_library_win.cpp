#include "dynamic_load_library.h"
#include "simple_os_defs.h"
#include "module_util.h"
#include <filesystem>
typedef struct LibraryInfoWin_t {
    HMODULE lib_handle;
}LibraryInfoWin_t;
namespace utilpp {
    void* simple_dlopen(std::filesystem::path& libPath) {
        HMODULE lib_handle{ NULL };
        if (libPath.is_absolute()) {
            lib_handle = ::LoadLibraryExW((LPCWSTR)libPath.u16string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (lib_handle == NULL)
            {
                return nullptr;
            }
        }
        else {
            lib_handle = ::LoadLibraryExW((LPCWSTR)libPath.u16string().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if (lib_handle == NULL)
            {
                return nullptr;
            }
        }
        auto out = new LibraryInfoWin_t();
        out->lib_handle = lib_handle;
        return out;
    }
}

void* simple_dlopen(const char* lib_name)
{
    std::filesystem::path libPath(lib_name);
    return utilpp::simple_dlopen(libPath);
}

void* simple_dlsym(void* handle, const char* func_name)
{
    if (!handle) {
        return NULL;
    }
    auto pLibraryInfo = reinterpret_cast<LibraryInfoWin_t*>(handle);
    FARPROC func_ptr = ::GetProcAddress(pLibraryInfo->lib_handle, func_name);
    if (func_ptr == nullptr)
    {
        return NULL;
    }
    return func_ptr;
}

bool simple_dlclose(void* handle)
{
    if (!handle) {
        return NULL;
    }
    auto pLibraryInfo = reinterpret_cast<LibraryInfoWin_t*>(handle);
    const BOOL rc = ::FreeLibrary(pLibraryInfo->lib_handle);
    if (rc == 0)
    {
        return false;
    }
    delete pLibraryInfo;
    return true;
}
