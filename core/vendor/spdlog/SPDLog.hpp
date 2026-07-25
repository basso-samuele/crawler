#pragma once

#include <memory>
#include <spdlog/logger.h>

#include "Log.hpp"

namespace Crawler
{

class SPDLog : public Log
{
private:
    std::shared_ptr<spdlog::logger> p_Logger;

public:
    SPDLog();
    ~SPDLog() override;

private:
    void p_TraceImpl(std::string message) override;
    void p_DebugImpl(std::string message) override;
    void p_WarnImpl(std::string message) override;
    void p_ErrorImpl(std::string message) override;
};

}