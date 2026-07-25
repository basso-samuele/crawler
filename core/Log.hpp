#pragma once

#include <memory>
#include <spdlog/logger.h>

#include "Singleton.hpp"

namespace Crawler
{

class Log : public Singleton<Log>
{
private:
    std::shared_ptr<spdlog::logger> p_Logger;

public:
    Log();
    ~Log() override;

    std::shared_ptr<spdlog::logger> Logger();
};

}

#ifdef DEBUG

#define TRACE(...) Crawler::Log::GetInstance().Logger()->trace(__VA_ARGS__)
#define DEBUG(...) Crawler::Log::GetInstance().Logger()->debug(__VA_ARGS__)
#define INFO(...) Crawler::Log::GetInstance().Logger()->info(__VA_ARGS__)
#define WARN(...) Crawler::Log::GetInstance().Logger()->warn(__VA_ARGS__)
#define ERROR(...) Crawler::Log::GetInstance().Logger()->error(__VA_ARGS__)
#define CRITICAL(...) Crawler::Log::GetInstance().Logger()->critical(__VA_ARGS__)

#define ASSERT(condition, message) assert(condition &&message)

#else

#define TRACE(...)
#define DEBUG(...)
#define INFO(...)
#define WARN(...)
#define ERROR(...)
#define CRITICAL(...)

#define ASSERT(condition, message)

#endif