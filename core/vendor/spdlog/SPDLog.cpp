#include "SPDLog.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <format>
#include <string>
#include <chrono>
#include <memory>

namespace Crawler
{

SPDLog::SPDLog() {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        std::format("{}{:%Y%m%d-%H%M%S}", CRAWLERLOGDIR, std::chrono::system_clock::now())));

    sinks[0]->set_pattern("[%T] [%t] [%n] [%l]: %v");
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks[1]->set_pattern("%^[%T] [%t] [%n] [%l]: %v%$");

    this->p_Logger = std::make_shared<spdlog::logger>("Crawler", begin(sinks), end(sinks));
    this->p_Logger->set_level(spdlog::level::trace);
    this->p_Logger->flush_on(spdlog::level::trace);
}

SPDLog::~SPDLog() {
    spdlog::shutdown();
}

void SPDLog::p_TraceImpl(std::string message) {
    this->p_Logger->trace(message);
}

void SPDLog::p_DebugImpl(std::string message) {
    this->p_Logger->debug(message);
}

void SPDLog::p_WarnImpl(std::string message) {
    this->p_Logger->warn(message);
}

void SPDLog::p_ErrorImpl(std::string message) {
    this->p_Logger->error(message);
}

}