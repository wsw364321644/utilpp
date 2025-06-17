#ifdef UTILPP_ENABLE_LOG
#include <LoggerHelper.h>
#else
#define SIMPLELOG_LOGGER_INFO(name ,...)(__VA_ARGS__)
#define SIMPLELOG_LOGGER_WARN(name ,...) (__VA_ARGS__)
#define SIMPLELOG_LOGGER_ERROR(name ,...) (__VA_ARGS__)
#endif // UTILPP_ENABLE_LOG
