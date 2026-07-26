#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined(MEMORY_POOL_EXPORTS)
#define MEMORY_POOL_EXPORT __declspec( dllexport ) 
#elif defined(MEMORY_POOL_NODLL )
#define MEMORY_POOL_EXPORT 
#else
#define MEMORY_POOL_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( MEMORY_POOL_EXPORTS )
#define MEMORY_POOL_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define MEMORY_POOL_EXPORT 
#endif 
#else // !WIN32
#if defined( MEMORY_POOL_EXPORTS )
#define MEMORY_POOL_EXPORT 
#else
#define MEMORY_POOL_EXPORT 
#endif 
#endif
#else
#define MEMORY_POOL_EXPORT 
#endif