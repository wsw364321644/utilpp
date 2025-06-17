#pragma once
#include <simple_os_defs.h>
#ifdef WIN32
#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#define HOST_NAME_MAX 256
#else
#include <unistd.h>
#endif 
