#include "LoggerHelper.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <module_util.h>
#include <mutex>
#include <variant>
#include <filesystem>
#include <format>
static std::mutex CreateLoggerMtx;



template <typename T, 
    typename std::enable_if<std::is_same<typename std::decay<T>::type, std::filesystem::path>::value>::type* = nullptr>
spdlog::sink_ptr GetSamePathSink(T&& path) {
    spdlog::sink_ptr psink;
    spdlog::details::registry::instance().apply_all(
        [&path, &psink](const std::shared_ptr<spdlog::logger> itr) {
            auto sinks = itr->sinks();
            for (auto& sinkitr : sinks) {
                auto pRotateSink = std::dynamic_pointer_cast<spdlog::sinks::rotating_file_sink_mt>(sinkitr);
                if (pRotateSink) {
                    if (std::filesystem::path(pRotateSink->filename()) == path) {
                        psink = sinkitr;
                        return;
                    }
                }
            }
        }
    );
    return psink;
}

std::shared_ptr<FLoggerWrapper> InternalCreateAsyncLogger(const LoggerSetting_t& setting)
{
    std::unique_lock lock(CreateLoggerMtx);
    auto logger = spdlog::get(setting.Name);
    if (logger)
    {
        return std::make_shared<FLoggerWrapper>(logger);
    }
    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(8192, 1);
    }
    std::vector<spdlog::sink_ptr> sinks;
    for (auto& pinfo : setting.Infos) {
        auto pStdoutLoggerInfo= std::dynamic_pointer_cast<StdoutLoggerInfo_t>(pinfo);
        if (pStdoutLoggerInfo) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            continue;
        }
        auto pMSVCLoggerInfo = std::dynamic_pointer_cast<MSVCLoggerInfo_t>(pinfo);
        if (pMSVCLoggerInfo) {
            sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
            continue;
        }
        auto pRotatingLoggerInfo = std::dynamic_pointer_cast<RotatingLoggerInfo_t>(pinfo);
        if (pRotatingLoggerInfo) {
            std::filesystem::path p(pRotatingLoggerInfo->FilePath);
            if (!p.has_filename()) {
                continue;
            }
            auto pSamePathSink=GetSamePathSink(std::filesystem::path(pRotatingLoggerInfo->FilePath));
            if (pSamePathSink) {
                sinks.push_back(pSamePathSink);
            }
            else {
                sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    pRotatingLoggerInfo->FilePath, pRotatingLoggerInfo->FileSize, pRotatingLoggerInfo->FileNum, pRotatingLoggerInfo->RotateOnOpen));
            }
            continue;
        }
    }

    logger = std::make_shared<spdlog::async_logger>(setting.Name, sinks.begin(), sinks.end(),
        spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    if (!setting.Name.empty()) {
        logger->set_pattern("[%D %T %z] [%n] [%^---%L---%$] [%s:%!] %v");
    }
    spdlog::register_logger(logger);

    return std::make_shared<FLoggerWrapper>(logger);
}

std::shared_ptr<FLoggerWrapper> CreateDefaultFileLogger(std::string name) {
    size_t size{ 0 };
    util_exe_wpath(nullptr, &size);
    auto pathstr = new wchar_t[++size];
    util_exe_wpath(pathstr, &size);
    std::filesystem::path path(pathstr);
    delete[] pathstr;
    auto stru8=path.stem().u8string();
    stru8.append(u8".log");

    std::string str(stru8.cbegin(), stru8.cend());
    auto pWrapper = InternalCreateAsyncLogger({ name, {std::make_shared<RotatingLoggerInfo_t>(str),std::make_shared<MSVCLoggerInfo_t>(),std::make_shared<StdoutLoggerInfo_t>()} });
    if (name.empty()) {
        spdlog::set_default_logger(pWrapper->Logger);
    }
#ifdef NDEBUG
    pWrapper->set_level(spdlog::level::warn);
#else
    pWrapper->set_level(spdlog::level::trace);
#endif // DEBUG

    return pWrapper;
}
std::shared_ptr<FLoggerWrapper> CreateAsyncLogger(const LoggerSetting_t& setting)
{
    auto logger = spdlog::get(setting.Name);
    if (logger){
        return std::make_shared<FLoggerWrapper>(logger);
    }
    return InternalCreateAsyncLogger(setting);
}

std::shared_ptr<FLoggerWrapper> GetLogger(std::string name)
{
    auto logger = spdlog::get(name);
    if (logger)
    {
        return std::make_shared<FLoggerWrapper>(logger);
    }
    return CreateDefaultFileLogger(name);
}

std::shared_ptr<FLoggerWrapper> GetLogger(std::nullptr_t  ptr)
{
    return GetLogger("");
}

