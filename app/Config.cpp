#include "Config.hpp"

#include <fstream>
#include <sstream>

namespace {

std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

DiagnosticConfig Config::load(const std::string& path)
{
    DiagnosticConfig config;

    std::ifstream file(path);
    if (!file) {
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const auto key = trim(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));

        if (key == "Port") {
            config.port = static_cast<std::uint16_t>(std::stoul(value));
        } else if (key == "LogLevel") {
            config.log_level = value;
        } else if (key == "VIN") {
            config.vin = value;
        }
    }

    return config;
}
