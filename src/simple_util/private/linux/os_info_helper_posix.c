#include "os_info_helper.h"
#include <simple_os_defs.h>
#include <sys/utsname.h>
void GetOsInfo(OSInfo_t* pinfo)
{
    OSInfo_t& info = *pinfo;
    int ires=0;
    struct utsname out_utsname;
    ires=uname(&out_utsname);
    int major=0;
    for (int i = 0; i < strlen(out_utsname.release), i++) {
        if (out_utsname.release[i] == ".") {
            break;
        }
        major = major * 10 + int(out_utsname.release[i]) - int('0');
    }
    if (major == 6) {
        info.OSCoreType = EOSCoreType::OSCT_Linux6x;
    }
}