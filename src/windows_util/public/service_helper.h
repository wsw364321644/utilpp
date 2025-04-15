#pragma once
#include "windows_util_export_defs.h"
WINDOWS_UTIL_API void  DoInstallSvc(const char* svcname, const char* exepath);

/*
* @brief Starts the service if possible.
* @param svcname The name of the service to start.
* @param argc The number of arguments.
* @param argv The arguments to pass to the service.
* @return None
*/
WINDOWS_UTIL_API void  DoStartSvc(const char* svcname, int argc, const char* const* argv);

/*
* @brief Updates the service DACL to grant start, stop, delete, and read
*        control access to the Guest account.
* @param svcname The name of the service to update.
*/
WINDOWS_UTIL_API void  DoUpdateSvcDacl(const char* svcname);

/*
* @brief Stops the service if possible.
*/
WINDOWS_UTIL_API void  DoStopSvc(const char* svcname);

typedef struct ServiceConfig_t {
	char* BinaryPath;
}ServiceConfig_t;
/*
* @brief Retrieves and displays the current service configuration.
* @param svcname The name of the service to query.
* @param outconfig A pointer to a ServiceConfig_t structure that will
*                  receive the service configuration.
*/
WINDOWS_UTIL_API void  DoQuerySvc(const char* svcname, ServiceConfig_t** outconfig);
WINDOWS_UTIL_API void  FreeQueryData(ServiceConfig_t** config);

/*
* @brief Updates the service description to "This is a test description".
*/
WINDOWS_UTIL_API void  DoUpdateSvcDesc(const char* svcname);
WINDOWS_UTIL_API void  DoDisableSvc(const char* svcname);
WINDOWS_UTIL_API void  DoEnableSvc(const char* svcname);
WINDOWS_UTIL_API void  DoDeleteSvc(const char* svcname);

WINDOWS_UTIL_API bool  IsServiceExist(const char* svcname);
WINDOWS_UTIL_API bool  IsServiceRunning(const char* svcname);