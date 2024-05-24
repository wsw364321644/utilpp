#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_EXPORT __declspec( dllexport ) 
#elif defined( SIMPLE_UTIL_API_NODLL )
#define SIMPLE_UTIL_EXPORT 
#else
#define SIMPLE_UTIL_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define SIMPLE_UTIL_EXPORT 
#endif 
#else // !WIN32
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_EXPORT 
#else
#define SIMPLE_UTIL_EXPORT 
#endif 
#endif
#else
#define SIMPLE_UTIL_EXPORT 
#endif