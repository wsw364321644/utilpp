#include "os_default_application.h"
#include "simple_os_defs.h"
#include <shellapi.h>
void open_url(const char* url_cstr) {
    ShellExecute(NULL, "open", url_cstr, NULL, NULL, SW_SHOWNORMAL);
}