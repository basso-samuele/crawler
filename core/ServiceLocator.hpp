#pragma once

#include <memory>

#include "Singleton.hpp"

#include "Log.hpp"
#include "vendor/spdlog/SPDLog.hpp"

namespace Crawler
{

class ServiceLocator : public Singleton<ServiceLocator>
{
private:
    std::shared_ptr<Log> p_Logger;

public:
    ServiceLocator()
    : p_Logger(std::make_shared<SPDLog>()) { }

    std::shared_ptr<Log> GetLogger() {
        return this->p_Logger;
    }
};

}