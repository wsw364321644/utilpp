#include "logger.h"
#include "dir_util.h"
#include "module_util.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <filesystem>
const char *Log_System = " **System** ";
const char *Log_Server = " **Server** ";
const char *Log_Client = " **Client** ";
const char *Log_Api = " **Api** ";
const char *Log_Packet = " **Packet** ";
const char *Log_File = " **File** ";
const char *Log_Game = " **Game** ";

struct GlobalParam
{
    std::string file_name;
    std::string format;
    std::string enable;
    std::string to_file;
    std::string to_stdout;
    std::string millisecond_width;
    size_t max_file_size;
    size_t max_file_count;
};

static GlobalParam sParam = {
    "default.log",
    "%datetime %level: %msg",
    "true",
    "true",
    "false",
    "3",
    10485760,
    5};

static const char *Debug2Stdout = "true"; // "false";
static const char *Info2Stdout = "true";
static const char *Error2Stdout = "true";
static const char *Warning2Stdout = "true"; // "false";

static std::shared_ptr<spdlog::logger> sLogger;
void Logger::Init(const char *log_file)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file ? log_file : sParam.file_name, sParam.max_file_size, sParam.max_file_count));
    sLogger = std::make_shared<spdlog::logger>("runtime_logger", begin(sinks), end(sinks));
    sLogger->set_level(spdlog::level::debug);
    sLogger->flush_on(spdlog::level::warn);
}

void Logger::Exit()
{
    spdlog::drop_all();
}

std::shared_ptr<spdlog::logger> Logger::Instance()
{
    if (!sLogger)
    {
        char path[MAX_PATH];
        size_t size;
        util_dll_path(path, &size);
        std::filesystem::path logpath(path);
        logpath = logpath.replace_extension(".log");
        auto logpathu8 = logpath.u8string();
        Init((const char*)logpathu8.c_str());
    }
    return sLogger;
}