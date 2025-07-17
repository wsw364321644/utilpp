#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SYSTEM_INFO_API_EXPORTS )
#define SYSTEM_INFO_EXPORT __declspec( dllexport ) 
#elif defined( SYSTEM_INFO_API_NODLL )
#define SYSTEM_INFO_EXPORT 
#else
#define SYSTEM_INFO_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SYSTEM_INFO_API_EXPORTS )
#define SYSTEM_INFO_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define SYSTEM_INFO_EXPORT 
#endif 
#else // !WIN32
#if defined( SYSTEM_INFO_API_EXPORTS )
#define SYSTEM_INFO_EXPORT 
#else
#define SYSTEM_INFO_EXPORT 
#endif 
#endif
#else
#define SYSTEM_INFO_EXPORT 
#endif