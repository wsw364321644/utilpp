#pragma once
#include "logger_export_defs.h"
#include <string>
#include <memory>
#include <vector>
#include <format>
#include <spdlog/spdlog.h>

struct BaseLoggerInfo_t{
    virtual ~BaseLoggerInfo_t() {}
};
struct StdoutLoggerInfo_t: BaseLoggerInfo_t {
};
struct MSVCLoggerInfo_t : BaseLoggerInfo_t {
};

struct FileLoggerInfo_t : BaseLoggerInfo_t {
    FileLoggerInfo_t() {}
    FileLoggerInfo_t(std::string str) :FilePath(str) {}
    std::string FilePath;
};
struct RotatingLoggerInfo_t:FileLoggerInfo_t {
    RotatingLoggerInfo_t() {}
    RotatingLoggerInfo_t(std::string str) :FileLoggerInfo_t(str ) {}
    int FileSize{ 1024 * 1024 * 5 };
    int FileNum{1};
    bool RotateOnOpen{ false };
};

struct LoggerSetting_t{
    std::string Name;
    std::vector<std::shared_ptr<BaseLoggerInfo_t>> Infos;
};


class FLoggerWrapper {
public:
    FLoggerWrapper(std::shared_ptr<spdlog::logger> logger) :Logger(logger) {
    }
    ~FLoggerWrapper() {
    }

    template <typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->log(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->critical(fmt, std::forward<Args>(args)...);
    }

    void set_level(spdlog::level::level_enum log_level) {
        Logger->set_level(log_level);
    }

    template <typename... Args>
    void log(spdlog::source_loc loc, spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Logger->log(loc,lvl,fmt, std::forward<Args>(args)...);
    }

    void flush() {
        Logger->flush();
    }
    std::shared_ptr<spdlog::logger> Logger;
};
SIMPLE_LOGGER_EXPORT std::shared_ptr<FLoggerWrapper> CreateAsyncLogger(const LoggerSetting_t& setting);
SIMPLE_LOGGER_EXPORT std::shared_ptr<FLoggerWrapper> GetLogger(std::string_view name = "");
SIMPLE_LOGGER_EXPORT std::shared_ptr<FLoggerWrapper> GetLogger(std::nullptr_t ptr);
SIMPLE_LOGGER_EXPORT void ShutdownLogger();

#define SIMPLELOG_LOGGER_TRACE(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::trace, __VA_ARGS__)
#define SIMPLELOG_LOGGER_DEBUG(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::debug, __VA_ARGS__)
#define SIMPLELOG_LOGGER_INFO(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::info, __VA_ARGS__)
#define SIMPLELOG_LOGGER_WARN(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::warn, __VA_ARGS__)
#define SIMPLELOG_LOGGER_ERROR(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::err, __VA_ARGS__)
#define SIMPLELOG_LOGGER_CRIRICAL(name ,...) GetLogger(name)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::level_enum::critical, __VA_ARGS__)

