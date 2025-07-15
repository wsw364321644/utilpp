#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_EXPORT __declspec( dllexport ) 
#elif defined( CRYPTO_LIB_HELPER_API_NODLL )
#define CRYPTO_LIB_HELPER_EXPORT 
#else
#define CRYPTO_LIB_HELPER_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define CRYPTO_LIB_HELPER_EXPORT 
#endif 
#else // !WIN32
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_EXPORT 
#else
#define CRYPTO_LIB_HELPER_EXPORT 
#endif 
#endif
#else
#define CRYPTO_LIB_HELPER_EXPORT 
#endif

#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API extern "C" __declspec( dllexport ) 
#elif defined( CRYPTO_LIB_HELPER_API_NODLL )
#define CRYPTO_LIB_HELPER_API extern "C"
#else
#define CRYPTO_LIB_HELPER_API extern "C" __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API extern "C" __attribute__ ((visibility("default"))) 
#else
#define CRYPTO_LIB_HELPER_API extern "C" 
#endif 
#else // !WIN32
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API extern "C"  
#else
#define CRYPTO_LIB_HELPER_API extern "C" 
#endif 
#endif
#else
#if defined( _WIN32 ) && !defined( _X360 )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API  __declspec( dllexport ) 
#elif defined( CRYPTO_LIB_HELPER_API_NODLL )
#define CRYPTO_LIB_HELPER_API 
#else
#define CRYPTO_LIB_HELPER_API __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API __attribute__ ((visibility("default"))) 
#else
#define CRYPTO_LIB_HELPER_API 
#endif 
#else // !WIN32
#if defined( CRYPTO_LIB_HELPER_API_EXPORTS )
#define CRYPTO_LIB_HELPER_API
#else
#define CRYPTO_LIB_HELPER_API
#endif 
#endif
#endif