#pragma once
#ifndef __cplusplus
#include <stdbool.h>
#endif 
#ifdef __cplusplus
extern "C" {
#endif 
	
void __stdcall DoInstallSvc(const char* svcname,const char* exepath);
void __stdcall DoStartSvc(const char* svcname,int argc, const char**argv);
void __stdcall DoUpdateSvcDacl(const char* svcname);
void __stdcall DoStopSvc(const char* svcname);

void __stdcall DoQuerySvc(const char* svcname);
void __stdcall DoUpdateSvcDesc(const char* svcname);
void __stdcall DoDisableSvc(const char* svcname);
void __stdcall DoEnableSvc(const char* svcname);
void __stdcall DoDeleteSvc(const char* svcname);

#ifdef __cplusplus
}
#endif 