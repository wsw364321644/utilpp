#include "HOOK/hook_synchronized.h"
#include <cstdio>
#include <windows_helper.h>

HANDLE create_mutex_plus_id(const char* name, DWORD id, BOOL is_app)
{
    char new_name[64]{ 0 };
    std::snprintf(new_name, 64, "%s%lu", name, id);

    return create_mutex(new_name, is_app);
}

HANDLE open_mutex_plus_id(const char* name, DWORD id, BOOL is_app)
{
    char new_name[64]{ 0 };
    std::snprintf(new_name, 64, "%s%lu", name, id);

    return open_mutex(new_name, is_app);
}

HANDLE create_event_plus_id(const char* name, DWORD id, BOOL is_app)
{
    char new_name[64];
    std::snprintf(new_name, 64, "%s%lu", name, id);
    return create_event(new_name, is_app);
}

HANDLE open_event_plus_id(const char* name, DWORD id, BOOL is_app)
{
    char new_name[64];
    std::snprintf(new_name, 64, "%s%lu", name, id);
    return open_event(new_name, is_app);
}
