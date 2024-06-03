#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SIMPLE_NET_API_EXPORTS )
#define SIMPLE_NET_EXPORT __declspec( dllexport ) 
#elif defined( SIMPLE_NET_API_NODLL )
#define SIMPLE_NET_EXPORT 
#else
#define SIMPLE_NET_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SIMPLE_NET_API_EXPORTS )
#define SIMPLE_NET_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define SIMPLE_NET_EXPORT 
#endif 
#else // !WIN32
#if defined( SIMPLE_NET_API_EXPORTS )
#define SIMPLE_NET_EXPORT 
#else
#define SIMPLE_NET_EXPORT 
#endif 
#endif
#else
#define SIMPLE_NET_EXPORT 
#endif