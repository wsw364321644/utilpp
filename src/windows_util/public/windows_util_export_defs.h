#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API extern "C" __declspec( dllexport ) 
#elif defined( WINDOWS_UTIL_NODLL )
#define WINDOWS_UTIL_API extern "C"
#else
#define WINDOWS_UTIL_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define WINDOWS_UTIL_API extern "C" 
#endif 
#else // !WIN32
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API extern "C"  
#else
#define WINDOWS_UTIL_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API  __declspec( dllexport ) 
#elif defined( WINDOWS_UTIL_NODLL )
#define WINDOWS_UTIL_API 
#else
#define WINDOWS_UTIL_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API __attribute__ ((visibility("default"))) 
#else
#define WINDOWS_UTIL_API 
#endif 
#else // !WIN32
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_API
#else
#define WINDOWS_UTIL_API
#endif 
#endif
#endif



#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_EXPORT __declspec( dllexport ) 
#elif defined( WINDOWS_UTIL_NODLL )
#define WINDOWS_UTIL_EXPORT 
#else
#define WINDOWS_UTIL_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define WINDOWS_UTIL_EXPORT 
#endif 
#else // !WIN32
#if defined( WINDOWS_UTIL_EXPORTS )
#define WINDOWS_UTIL_EXPORT 
#else
#define WINDOWS_UTIL_EXPORT 
#endif 
#endif
#else
#define WINDOWS_UTIL_EXPORT 
#endif