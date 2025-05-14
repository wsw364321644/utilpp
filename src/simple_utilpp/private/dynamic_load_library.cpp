#include "dynamic_load_library.h"
#include "string_convert.h"

void* simple_dlopen(std::u8string_view lib_name)
{
    std::string t = ConvertU8ViewToString(lib_name);
    return simple_dlopen(t.c_str());
}

void* simple_dlsym(void* handle, std::string_view func_name)
{
    return simple_dlsym(handle, std::string(func_name).c_str());
}