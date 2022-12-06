#pragma once

#include <stdint.h>
#include <stdbool.h>
typedef uint32_t HSDKRequest;

#ifdef __cplusplus
    extern "C" {
#endif

#define SONKWO_CALLTYPE __cdecl
#define PIPE_NAME_MAX   256


#define SONKWO_SVCNAME "Sonkwo Client Service"
        // the pipe for sdk and client runtime
#ifdef _WIN32 
#define SONKWO_RUNTIME_PIPE   "\\\\.\\pipe\\sonkwo_runtime_pipe"
#define SONKWO_API_PIPE "\\\\.\\pipe\\sonkwo_api_pipe"
#define SONKWO_API_EVENT "sonkwo_api_event"
#define SONKWO_APP_INSTALL_DIR "AppInstallDirectory"
#define SONKWO_MY_DOCUMENTS "WinMyDocuments"
#define SONKWO_APP_DATA_LOCAL "WinAppDataLocal"
#define SONKWO_APP_DATA_LOCAL_LOW "WinAppDataLocalLow"
#define SONKWO_APP_DATA_ROAMING "WinAppDataRoaming"
#define SONKWO_SAVED_GAMES "WinSavedGames"
#define SONKWO_GAMEPATH_MAPPING "Global\\SonkwoFileMappingObject"
#define SONKWO_CLIENT_EVENT "Global\\SonkwoClentEvent"
#define SONKWO_STARTGAME_OUT_EVENT "Global\\SonkwoStartGameOutEvent"
#define BUF_SIZE 1024


#define RUNTIME_TEST 1

#if RUNTIME_TEST
#define SONKWO_EXE_ROOTKEY HKEY_CURRENT_USER
#define SONKWO_EXE_REGKEY "SOFTWARE\\sonkwo"
#else
#define SONKWO_EXE_ROOTKEY HKEY_LOCAL_MACHINE
#define SONKWO_EXE_REGKEY "SOFTWARE\\Classes\\sonkwo"
#endif
#define SONKWO_EXE_VALUE_NAME "URL Protocol"
#else
#define SONKWO_RUNTIME_PIPE  "/tmp/sonkwo.runtime"
#endif
       
#ifdef __cplusplus
    }
#endif
