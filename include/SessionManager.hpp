#pragma once

#include <cstdint>
#include <string>

enum class DiagnosticSession : std::uint8_t {
    Default = 0x01,
    Extended = 0x03,
};

class SessionManager {
public:
    DiagnosticSession current() const;
    bool change(std::uint8_t sub_function);
    bool is_write_allowed() const;
    std::string current_name() const;

private:
    DiagnosticSession current_{DiagnosticSession::Default};
};
