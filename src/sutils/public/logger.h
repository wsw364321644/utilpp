/**
 *  logger.h
 */

#pragma once

#include <spdlog/spdlog.h>

extern const char *Log_System;
extern const char *Log_Server;
extern const char *Log_Client;
extern const char *Log_Api;
extern const char *Log_Packet;
extern const char *Log_File;
extern const char *Log_Game;

class Logger
{
public:
    static void Init(const char *log_file);
    static void Exit();
    static std::shared_ptr<spdlog::logger> Instance();
};

#define LOG_WARNING Logger::Instance()->warn
#define LOG_INFO Logger::Instance()->info
#define LOG_DEBUG Logger::Instance()->debug
#define LOG_ERROR Logger::Instance()->error
