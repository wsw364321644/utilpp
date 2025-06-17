#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API extern "C" __declspec( dllexport ) 
#elif defined( STEAM_MSG_API_NODLL )
#define STEAM_MSG_API extern "C"
#else
#define STEAM_MSG_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define STEAM_MSG_API extern "C" 
#endif 
#else // !WIN32
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API extern "C"  
#else
#define STEAM_MSG_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API  __declspec( dllexport ) 
#elif defined( STEAM_MSG_API_NODLL )
#define STEAM_MSG_API 
#else
#define STEAM_MSG_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API __attribute__ ((visibility("default"))) 
#else
#define STEAM_MSG_API 
#endif 
#else // !WIN32
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_API
#else
#define STEAM_MSG_API
#endif 
#endif
#endif


#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_EXPORT __declspec( dllexport ) 
#elif defined( STEAM_MSG_API_NODLL )
#define STEAM_MSG_EXPORT 
#else
#define STEAM_MSG_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define STEAM_MSG_EXPORT 
#endif 
#else // !WIN32
#if defined( STEAM_MSG_API_EXPORTS )
#define STEAM_MSG_EXPORT 
#else
#define STEAM_MSG_EXPORT 
#endif 
#endif
#else
#define STEAM_MSG_EXPORT 
#endif