#pragma once

#include <chrono>

#include "ServiceLocator.hpp"

namespace Crawler
{

class ScopedTimer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> p_Start;

    void p_PrintDuration(std::chrono::time_point<std::chrono::high_resolution_clock> end) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - this->p_Start);
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(ns);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(us);
        ServiceLocator::GetInstance().GetLogger()->Trace("Scoped timer duration: {} ({}, {}).", ns, us, ms);
    }

public:
    ScopedTimer()
    : p_Start(std::chrono::high_resolution_clock::now()) { }

    ~ScopedTimer() {
        std::chrono::time_point<std::chrono::high_resolution_clock> end
            = std::chrono::high_resolution_clock::now();
        this->p_PrintDuration(end);
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer&& operator=(ScopedTimer&&) = delete;
};

}