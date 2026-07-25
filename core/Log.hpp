#pragma once

#include <format>
#include <string>

namespace Crawler
{

class Log
{
public:
    Log() = default;
    virtual ~Log() = default;

    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    Log(Log&&) = delete;
    Log& operator=(Log&&) = delete;

    template<typename... Args>
    void Trace(std::format_string<Args...> fmt, Args&&... args) {
        this->p_TraceImpl(std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Debug(std::format_string<Args...> fmt, Args&&... args) {
        this->p_DebugImpl(std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Warn(std::format_string<Args...> fmt, Args&&... args) {
        this->p_WarnImpl(std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Error(std::format_string<Args...> fmt, Args&&... args) {
        this->p_ErrorImpl(std::format(fmt, std::forward<Args>(args)...));
    }

private:
    virtual void p_TraceImpl(std::string) = 0;
    virtual void p_DebugImpl(std::string) = 0;
    virtual void p_WarnImpl(std::string) = 0;
    virtual void p_ErrorImpl(std::string) = 0;
};

}