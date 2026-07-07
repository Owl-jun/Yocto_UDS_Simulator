#pragma once

#include <cstdint>
#include <string>

struct DiagnosticConfig {
    std::uint16_t port{5000};
    std::string log_level{"INFO"};
    std::string vin{"KMH00000000000000"};
};

class Config {
public:
    static DiagnosticConfig load(const std::string& path);
};
