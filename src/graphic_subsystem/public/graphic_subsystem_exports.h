#pragma once

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_EXPORT __declspec( dllexport ) 
#elif defined( GRAPHIC_SUBSYSTEM_NODLL )
#define GRAPHIC_SUBSYSTEM_EXPORT 
#else
#define GRAPHIC_SUBSYSTEM_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define GRAPHIC_SUBSYSTEM_EXPORT 
#endif 
#else // !WIN32
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_EXPORT 
#else
#define GRAPHIC_SUBSYSTEM_EXPORT 
#endif 
#endif
#else
#define GRAPHIC_SUBSYSTEM_EXPORT 
#endif

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API extern "C" __declspec( dllexport ) 
#elif defined( GRAPHIC_SUBSYSTEM_NODLL )
#define GRAPHIC_SUBSYSTEM_API extern "C"
#else
#define GRAPHIC_SUBSYSTEM_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define GRAPHIC_SUBSYSTEM_API extern "C" 
#endif 
#else // !WIN32
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API extern "C"  
#else
#define GRAPHIC_SUBSYSTEM_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API  __declspec( dllexport ) 
#elif defined( GRAPHIC_SUBSYSTEM_NODLL )
#define GRAPHIC_SUBSYSTEM_API 
#else
#define GRAPHIC_SUBSYSTEM_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API __attribute__ ((visibility("default"))) 
#else
#define GRAPHIC_SUBSYSTEM_API 
#endif 
#else // !WIN32
#if defined( GRAPHIC_SUBSYSTEM_EXPORTS )
#define GRAPHIC_SUBSYSTEM_API
#else
#define GRAPHIC_SUBSYSTEM_API
#endif 
#endif
#endif