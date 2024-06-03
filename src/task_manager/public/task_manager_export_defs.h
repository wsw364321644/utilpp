#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( TASK_MANAGER_API_EXPORTS )
#define TASK_MANAGER_EXPORT __declspec( dllexport ) 
#elif defined( TASK_MANAGER_API_NODLL )
#define TASK_MANAGER_EXPORT 
#else
#define TASK_MANAGER_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( TASK_MANAGER_API_EXPORTS )
#define TASK_MANAGER_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define TASK_MANAGER_EXPORT 
#endif 
#else // !WIN32
#if defined( TASK_MANAGER_API_EXPORTS )
#define TASK_MANAGER_EXPORT 
#else
#define TASK_MANAGER_EXPORT 
#endif 
#endif
#else
#define TASK_MANAGER_EXPORT 
#endif