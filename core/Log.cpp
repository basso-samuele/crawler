#include "Log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <format>

namespace Crawler
{

Log::Log()
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        std::format("{}{:%Y%m%d-%H%M%S}", LOG_DIR, std::chrono::system_clock::now())));
    sinks[0]->set_pattern("[%T] [%t] [%n] [%l]: %v");
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks[1]->set_pattern("%^[%T] [%t] [%n] [%l]: %v%$");
    this->p_Logger = std::make_shared<spdlog::logger>("Core", begin(sinks), end(sinks));
    this->p_Logger->set_level(spdlog::level::trace);
    this->p_Logger->flush_on(spdlog::level::trace);
}

Log::~Log()
{
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> Log::Core()
{
    return this->p_Logger;
}

}