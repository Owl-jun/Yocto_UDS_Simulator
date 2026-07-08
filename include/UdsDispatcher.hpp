#pragma once

#include "DidManager.hpp"
#include "DtcManager.hpp"
#include "SessionManager.hpp"

#include <string>

class UdsDispatcher {
public:
    UdsDispatcher(DidManager& did_manager, DtcManager& dtc_manager, SessionManager& session_manager);

    std::string handle_line(const std::string& request_line);

private:
    ByteVector dispatch(const ByteVector& request);
    ByteVector handle_session_control(const ByteVector& request);
    ByteVector handle_ecu_reset(const ByteVector& request);
    ByteVector handle_clear_diagnostic_information(const ByteVector& request);
    ByteVector handle_read_did(const ByteVector& request);
    ByteVector handle_write_did(const ByteVector& request);
    ByteVector handle_read_dtc_information(const ByteVector& request);
    ByteVector handle_security_access(const ByteVector& request);
    ByteVector handle_routine_control(const ByteVector& request);

    static ByteVector negative_response(std::uint8_t sid, std::uint8_t nrc);

    DidManager& did_manager_;
    DtcManager& dtc_manager_;
    SessionManager& session_manager_;
    bool security_seed_issued_{false};
    bool security_unlocked_{false};
    bool self_test_running_{false};
};
