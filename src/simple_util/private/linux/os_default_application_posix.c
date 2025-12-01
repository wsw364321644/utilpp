#include "os_default_application.h"
#include "simple_os_defs.h"
void open_url(const char* url_cstr) {
    char command[2048];
    snprintf(command, sizeof(command), "xdg-open %s ", url_cstr)
    system(command.c_str());
}