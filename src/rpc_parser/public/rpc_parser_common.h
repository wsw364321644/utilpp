#pragma once
#ifdef __cplusplus
#if defined( _WIN32 ) && !defined( _X360 )
#if defined(RPC_PARSER_EXPORTS)
#define RPC_PARSER_EXPORT __declspec( dllexport ) 
#elif defined(RPC_PARSER_NODLL )
#define RPC_PARSER_EXPORT 
#else
#define RPC_PARSER_EXPORT  __declspec( dllimport ) 
#endif 
#elif defined( GNUC )
#if defined( RPC_PARSER_EXPORTS )
#define RPC_PARSER_EXPORT  __attribute__ ((visibility("default"))) 
#else
#define RPC_PARSER_EXPORT 
#endif 
#else // !WIN32
#if defined( RPC_PARSER_EXPORTS )
#define RPC_PARSER_EXPORT 
#else
#define RPC_PARSER_EXPORT 
#endif 
#endif
#else
#define RPC_PARSER_EXPORT 
#endif