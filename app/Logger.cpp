#include "Logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

namespace {

void log(const std::string& level, const std::string& message)
{
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);

    std::cout << std::put_time(std::localtime(&now_time), "%F %T")
              << " [" << level << "] " << message << '\n';
}

} // namespace

void Logger::info(const std::string& message)
{
    log("INFO", message);
}

void Logger::error(const std::string& message)
{
    log("ERROR", message);
}
