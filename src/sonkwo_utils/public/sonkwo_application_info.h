#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

    bool IsSonkwoAppStarted();
    bool UpdateInfoBySonkwoDir(const char*);
    bool DeleteSonkwoInfo();
    ///@param[out] path   buf could be null
    ///@param[in, out] length  in buf length  out string length
    bool GetSonkwoDir(char*const* path, uint32_t* length);
    bool RunSonkwoClient();
#ifdef __cplusplus
}
#endif


namespace sonkwo
{
    
}