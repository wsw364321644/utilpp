#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API extern "C" __declspec( dllexport ) 
#elif defined( SIMPLE_UTIL_API_NODLL )
#define SIMPLE_UTIL_API extern "C"
#else
#define SIMPLE_UTIL_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define SIMPLE_UTIL_API extern "C" 
#endif 
#else // !WIN32
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API extern "C"  
#else
#define SIMPLE_UTIL_API extern "C" 
#endif 
#endif
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API  __declspec( dllexport ) 
#elif defined( SIMPLE_UTIL_API_NODLL )
#define SIMPLE_UTIL_API 
#else
#define SIMPLE_UTIL_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API __attribute__ ((visibility("default"))) 
#else
#define SIMPLE_UTIL_API 
#endif 
#else // !WIN32
#if defined( SIMPLE_UTIL_API_EXPORTS )
#define SIMPLE_UTIL_API
#else
#define SIMPLE_UTIL_API
#endif 
#endif
#endif