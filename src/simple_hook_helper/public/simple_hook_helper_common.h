#pragma once

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_EXPORT __declspec( dllexport ) 
#elif defined( HOOK_HELPER_NODLL )
#define HOOK_HELPER_EXPORT 
#else
#define HOOK_HELPER_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define HOOK_HELPER_EXPORT 
#endif 
#else // !WIN32
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_EXPORT 
#else
#define HOOK_HELPER_EXPORT 
#endif 
#endif
#else
#define HOOK_HELPER_EXPORT 
#endif

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API extern "C" __declspec( dllexport ) 
#elif defined( HOOK_HELPER_NODLL )
#define HOOK_HELPER_API extern "C"
#else
#define HOOK_HELPER_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define HOOK_HELPER_API extern "C" 
#endif 
#else // !WIN32
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API extern "C"  
#else
#define HOOK_HELPER_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API  __declspec( dllexport ) 
#elif defined( HOOK_HELPER_NODLL )
#define HOOK_HELPER_API 
#else
#define HOOK_HELPER_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API __attribute__ ((visibility("default"))) 
#else
#define HOOK_HELPER_API 
#endif 
#else // !WIN32
#if defined( HOOK_HELPER_EXPORTS )
#define HOOK_HELPER_API
#else
#define HOOK_HELPER_API
#endif 
#endif
#endif