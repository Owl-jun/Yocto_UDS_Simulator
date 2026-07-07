#include "SessionManager.hpp"

DiagnosticSession SessionManager::current() const
{
    return current_;
}

bool SessionManager::change(std::uint8_t sub_function)
{
    if (sub_function == static_cast<std::uint8_t>(DiagnosticSession::Default)) {
        current_ = DiagnosticSession::Default;
        return true;
    }

    if (sub_function == static_cast<std::uint8_t>(DiagnosticSession::Extended)) {
        current_ = DiagnosticSession::Extended;
        return true;
    }

    return false;
}

void SessionManager::reset()
{
    current_ = DiagnosticSession::Default;
}

bool SessionManager::is_write_allowed() const
{
    return current_ == DiagnosticSession::Extended;
}

std::string SessionManager::current_name() const
{
    switch (current_) {
    case DiagnosticSession::Default:
        return "default";
    case DiagnosticSession::Extended:
        return "extended";
    }

    return "unknown";
}
