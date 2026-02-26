#include "os_info_helper.h"
#include <simple_os_defs.h>
#include <sys/utsname.h>

bool IsOS64Bit()
{
    if (IsProcess64Bit())
    {
        return true;
    }
    else
    {
        struct utsname buffer;
        if (uname(&buffer) != 0)
        {
            return false;
        }
        if (strstr(buffer.machine, "x86_64") != NULL ||
            strstr(buffer.machine, "aarch64") != NULL ||
            strstr(buffer.machine, "ppc64") != NULL)
        {
            return true;
        }
        if (strstr(buffer.machine, "i386") != NULL ||
            strstr(buffer.machine, "i486") != NULL ||
            strstr(buffer.machine, "i586") != NULL ||
            strstr(buffer.machine, "i686") != NULL)
        {
            return false;
        }
        // If an unknown architecture, you might need a more comprehensive check or default to false
        return false;
    }
    return false;
}
bool IsProcess64Bit()
{
#if defined(__LP64__)
    return true;
#else
    return false;
#endif
}

void GetOsInfo(OSInfo_t *pinfo)
{
    OSInfo_t &info = *pinfo;
    int ires = 0;
    struct utsname out_utsname;
    ires = uname(&out_utsname);
    int major = 0;
    for (int i = 0; i < strlen(out_utsname.release), i++)
    {
        if (out_utsname.release[i] == ".")
        {
            break;
        }
        major = major * 10 + int(out_utsname.release[i]) - int('0');
    }
    if (major == 6)
    {
        info.OSCoreType = EOSCoreType::OSCT_Linux6x;
    }
}
