#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( SIMPLE_ARCHIVE_API_EXPORTS )
#define SIMPLE_ARCHIVE_EXPORT __declspec( dllexport ) 
#elif defined( SIMPLE_ARCHIVE_API_NODLL )
#define SIMPLE_ARCHIVE_EXPORT 
#else
#define SIMPLE_ARCHIVE_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( SIMPLE_ARCHIVE_API_EXPORTS )
#define SIMPLE_ARCHIVE_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define SIMPLE_ARCHIVE_EXPORT 
#endif 
#else // !WIN32
#if defined( SIMPLE_ARCHIVE_API_EXPORTS )
#define SIMPLE_ARCHIVE_EXPORT 
#else
#define SIMPLE_ARCHIVE_EXPORT 
#endif 
#endif
#else
#define SIMPLE_ARCHIVE_EXPORT 
#endif