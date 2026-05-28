#include "dynamic_load_library.h"
#include <string_convert.h>

void simple_dlsym_function_ptr(void* handle, const char* func_name, void** outptr)
{
    if (outptr == NULL) {
        return;
    }
    auto ptr= simple_dlsym(handle, func_name);
    if (!ptr) {
        *outptr = NULL;
    }
    else {
        *outptr = ptr;
    }
    return;
}


namespace utilpp {
    void* simple_dlopen(std::u8string_view lib_name)
    {
        std::filesystem::path libPath(lib_name);
        return simple_dlopen(libPath);
    }

    void* simple_dlopen(std::u16string_view lib_name)
    {
        std::filesystem::path libPath(lib_name);
        return simple_dlopen(libPath);
    }

    void* simple_dlsym(void* handle, std::string_view func_name)
    {
        return ::simple_dlsym(handle, std::string(func_name).c_str());
    }
}