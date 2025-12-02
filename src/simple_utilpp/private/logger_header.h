#ifdef UTILPP_ENABLE_LOG
#include <LoggerHelper.h>
#else
//#define SIMPLELOG_LOGGER_INFO(name ,...) (__VA_ARGS__)
#define SIMPLELOG_LOGGER_INFO(name ,...)
#define SIMPLELOG_LOGGER_WARN(name ,...)
#define SIMPLELOG_LOGGER_ERROR(name ,...)
#endif // UTILPP_ENABLE_LOG
