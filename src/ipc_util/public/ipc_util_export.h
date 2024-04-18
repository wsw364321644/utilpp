#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined(IPC_UTIL_EXPORTS)
#define IPC_EXPORT __declspec( dllexport ) 
#elif defined(IPC_UTIL_NODLL )
#define IPC_EXPORT 
#else
#define IPC_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( IPC_UTIL_EXPORTS )
#define IPC_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define IPC_EXPORT 
#endif 
#else // !WIN32
#if defined( IPC_UTIL_EXPORTS )
#define IPC_EXPORT 
#else
#define IPC_EXPORT 
#endif 
#endif
#else
#define IPC_EXPORT 
#endif

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API extern "C" __declspec( dllexport ) 
#elif defined( IPC_UTIL_NODLL )
#define IPC_API extern "C"
#else
#define IPC_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define IPC_API extern "C" 
#endif 
#else // !WIN32
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API extern "C"  
#else
#define IPC_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API  __declspec( dllexport ) 
#elif defined( IPC_UTIL_NODLL )
#define IPC_API 
#else
#define IPC_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API __attribute__ ((visibility("default"))) 
#else
#define IPC_API 
#endif 
#else // !WIN32
#if defined( IPC_UTIL_EXPORTS )
#define IPC_API
#else
#define IPC_API
#endif 
#endif
#endif