/**
 *  dll_win.c
 */

#include <Windows.h>
#include "logger.h"

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    //printf("hModule.%p lpReserved.%p \n", hinstDLL, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        //printf("Process attach. \n");
        sonkwo::Logger::Init("api.log");
        break;

    case DLL_PROCESS_DETACH:
        //printf("Process detach. \n");
        break;

    case DLL_THREAD_ATTACH:
        //printf("Thread attach. \n");
        break;

    case DLL_THREAD_DETACH:
        //printf("Thread detach. \n");
        break;
    }

    
    return (TRUE);
}
