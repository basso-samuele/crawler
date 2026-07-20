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

    std::shared_ptr<spdlog::logger> Core();
};

}

#ifdef DEBUG

#define CORE_TRACE(...) Crawler::Log::GetInstance().Core()->trace(__VA_ARGS__)
#define CORE_DEBUG(...) Crawler::Log::GetInstance().Core()->debug(__VA_ARGS__)
#define CORE_INFO(...) Crawler::Log::GetInstance().Core()->info(__VA_ARGS__)
#define CORE_WARN(...) Crawler::Log::GetInstance().Core()->warn(__VA_ARGS__)
#define CORE_ERROR(...) Crawler::Log::GetInstance().Core()->error(__VA_ARGS__)
#define CORE_CRITICAL(...) Crawler::Log::GetInstance().Core()->critical(__VA_ARGS__)

#define CORE_ASSERT(condition, message) assert(condition &&message)

#else

#define CORE_TRACE(...)
#define CORE_DEBUG(...)
#define CORE_INFO(...)
#define CORE_WARN(...)
#define CORE_ERROR(...)
#define CORE_CRITICAL(...)

#define CORE_ASSERT(condition, message)

#endif