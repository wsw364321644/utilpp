#ifdef UTILPP_ENABLE_LOG
#include <LoggerHelper.h>
#else
#define LOG_INFO
#define LOG_WARNING
#define LOG_ERROR
#define SIMPLELOG_LOGGER_INFO
#define SIMPLELOG_LOGGER_WARN
#define SIMPLELOG_LOGGER_ERROR
#endif // UTILPP_ENABLE_LOG
