#include "UdsDispatcher.hpp"

#include "Logger.hpp"

#include <iomanip>
#include <sstream>

namespace {

constexpr std::uint8_t kSidDiagnosticSessionControl = 0x10;
constexpr std::uint8_t kSidEcuReset = 0x11;
constexpr std::uint8_t kSidReadDtcInformation = 0x19;
constexpr std::uint8_t kSidReadDataByIdentifier = 0x22;
constexpr std::uint8_t kSidSecurityAccess = 0x27;
constexpr std::uint8_t kSidWriteDataByIdentifier = 0x2E;
constexpr std::uint8_t kSidRoutineControl = 0x31;

constexpr std::uint8_t kNrcIncorrectMessageLength = 0x13;
constexpr std::uint8_t kNrcSubFunctionNotSupported = 0x12;
constexpr std::uint8_t kNrcRequestSequenceError = 0x24;
constexpr std::uint8_t kNrcConditionsNotCorrect = 0x22;
constexpr std::uint8_t kNrcRequestOutOfRange = 0x31;
constexpr std::uint8_t kNrcInvalidKey = 0x35;
constexpr std::uint8_t kNrcServiceNotSupported = 0x11;

constexpr std::uint8_t kSecuritySeedHigh = 0x12;
constexpr std::uint8_t kSecuritySeedLow = 0x34;
constexpr std::uint8_t kSecurityKeyHigh = static_cast<std::uint8_t>(kSecuritySeedHigh ^ 0xFFU);
constexpr std::uint8_t kSecurityKeyLow = static_cast<std::uint8_t>(kSecuritySeedLow ^ 0xFFU);
constexpr std::uint16_t kSelfTestRoutineId = 0xFF00;

ByteVector parse_hex_line(const std::string& line)
{
    std::istringstream stream(line);
    std::string token;
    ByteVector bytes;

    while (stream >> token) {
        if (token.size() > 2 && token.rfind("0x", 0) == 0) {
            token = token.substr(2);
        }

        const auto value = std::stoul(token, nullptr, 16);
        if (value > 0xFF) {
            throw std::out_of_range("byte value out of range");
        }

        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    return bytes;
}

std::string to_hex_line(const ByteVector& bytes)
{
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0U) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<int>(bytes[i]);
    }

    return stream.str();
}

std::uint16_t read_did_from_request(const ByteVector& request)
{
    return static_cast<std::uint16_t>((request[1] << 8U) | request[2]);
}

} // namespace

UdsDispatcher::UdsDispatcher(DidManager& did_manager, SessionManager& session_manager)
    : did_manager_(did_manager)
    , session_manager_(session_manager)
{
}

std::string UdsDispatcher::handle_line(const std::string& request_line)
{
    try {
        const auto request = parse_hex_line(request_line);
        const auto response = dispatch(request);

        Logger::info("UDS request=" + to_hex_line(request) + " response=" + to_hex_line(response));
        return to_hex_line(response);
    } catch (const std::exception& error) {
        Logger::error(std::string("Invalid request: ") + error.what());
        return "7F 00 13";
    }
}

ByteVector UdsDispatcher::dispatch(const ByteVector& request)
{
    if (request.empty()) {
        return negative_response(0x00, kNrcIncorrectMessageLength);
    }

    switch (request[0]) {
    case kSidDiagnosticSessionControl:
        return handle_session_control(request);
    case kSidEcuReset:
        return handle_ecu_reset(request);
    case kSidReadDtcInformation:
        return handle_read_dtc_information(request);
    case kSidReadDataByIdentifier:
        return handle_read_did(request);
    case kSidSecurityAccess:
        return handle_security_access(request);
    case kSidWriteDataByIdentifier:
        return handle_write_did(request);
    case kSidRoutineControl:
        return handle_routine_control(request);
    default:
        return negative_response(request[0], kNrcServiceNotSupported);
    }
}

ByteVector UdsDispatcher::handle_session_control(const ByteVector& request)
{
    if (request.size() != 2U) {
        return negative_response(kSidDiagnosticSessionControl, kNrcIncorrectMessageLength);
    }

    if (!session_manager_.change(request[1])) {
        return negative_response(kSidDiagnosticSessionControl, kNrcRequestOutOfRange);
    }

    if (session_manager_.current() == DiagnosticSession::Default) {
        security_seed_issued_ = false;
        security_unlocked_ = false;
        self_test_running_ = false;
    }

    Logger::info("Session changed to " + session_manager_.current_name());
    return ByteVector {static_cast<std::uint8_t>(kSidDiagnosticSessionControl + 0x40U), request[1]};
}

ByteVector UdsDispatcher::handle_ecu_reset(const ByteVector& request)
{
    if (request.size() != 2U) {
        return negative_response(kSidEcuReset, kNrcIncorrectMessageLength);
    }

    if (request[1] != 0x01U && request[1] != 0x03U) {
        return negative_response(kSidEcuReset, kNrcSubFunctionNotSupported);
    }

    session_manager_.reset();
    security_seed_issued_ = false;
    security_unlocked_ = false;
    self_test_running_ = false;
    Logger::info("ECU reset simulated, session returned to default");
    return ByteVector {static_cast<std::uint8_t>(kSidEcuReset + 0x40U), request[1]};
}

ByteVector UdsDispatcher::handle_read_did(const ByteVector& request)
{
    if (request.size() != 3U) {
        return negative_response(kSidReadDataByIdentifier, kNrcIncorrectMessageLength);
    }

    const auto did = read_did_from_request(request);
    const auto value = did_manager_.read(did);
    if (!value) {
        return negative_response(kSidReadDataByIdentifier, kNrcRequestOutOfRange);
    }

    ByteVector response {static_cast<std::uint8_t>(kSidReadDataByIdentifier + 0x40U), request[1], request[2]};
    response.insert(response.end(), value->begin(), value->end());
    return response;
}

ByteVector UdsDispatcher::handle_write_did(const ByteVector& request)
{
    if (request.size() < 4U) {
        return negative_response(kSidWriteDataByIdentifier, kNrcIncorrectMessageLength);
    }

    if (!session_manager_.is_write_allowed()) {
        return negative_response(kSidWriteDataByIdentifier, kNrcConditionsNotCorrect);
    }

    const auto did = read_did_from_request(request);
    const ByteVector value(request.begin() + 3, request.end());
    if (!did_manager_.write(did, value)) {
        return negative_response(kSidWriteDataByIdentifier, kNrcRequestOutOfRange);
    }

    return ByteVector {static_cast<std::uint8_t>(kSidWriteDataByIdentifier + 0x40U), request[1], request[2]};
}

ByteVector UdsDispatcher::handle_read_dtc_information(const ByteVector& request)
{
    if (request.size() != 3U) {
        return negative_response(kSidReadDtcInformation, kNrcIncorrectMessageLength);
    }

    constexpr std::uint8_t kReportDtcByStatusMask = 0x02;
    if (request[1] != kReportDtcByStatusMask) {
        return negative_response(kSidReadDtcInformation, kNrcSubFunctionNotSupported);
    }

    constexpr std::uint8_t kStatusAvailabilityMask = 0xFF;
    constexpr std::uint32_t kExampleDtc = 0x123456;
    constexpr std::uint8_t kExampleStatus = 0x08;

    return ByteVector {
        static_cast<std::uint8_t>(kSidReadDtcInformation + 0x40U),
        request[1],
        kStatusAvailabilityMask,
        static_cast<std::uint8_t>((kExampleDtc >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((kExampleDtc >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(kExampleDtc & 0xFFU),
        kExampleStatus,
    };
}

ByteVector UdsDispatcher::handle_security_access(const ByteVector& request)
{
    if (request.size() != 2U && request.size() != 4U) {
        return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
    }

    if (session_manager_.current() != DiagnosticSession::Extended) {
        return negative_response(kSidSecurityAccess, kNrcConditionsNotCorrect);
    }

    constexpr std::uint8_t kRequestSeed = 0x01;
    constexpr std::uint8_t kSendKey = 0x02;

    if (request[1] == kRequestSeed) {
        if (request.size() != 2U) {
            return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
        }

        security_seed_issued_ = true;
        security_unlocked_ = false;
        return ByteVector {
            static_cast<std::uint8_t>(kSidSecurityAccess + 0x40U),
            request[1],
            kSecuritySeedHigh,
            kSecuritySeedLow,
        };
    }

    if (request[1] == kSendKey) {
        if (request.size() != 4U) {
            return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
        }

        if (!security_seed_issued_) {
            return negative_response(kSidSecurityAccess, kNrcRequestSequenceError);
        }

        if (request[2] != kSecurityKeyHigh || request[3] != kSecurityKeyLow) {
            security_unlocked_ = false;
            return negative_response(kSidSecurityAccess, kNrcInvalidKey);
        }

        security_unlocked_ = true;
        return ByteVector {static_cast<std::uint8_t>(kSidSecurityAccess + 0x40U), request[1]};
    }

    return negative_response(kSidSecurityAccess, kNrcSubFunctionNotSupported);
}

ByteVector UdsDispatcher::handle_routine_control(const ByteVector& request)
{
    if (request.size() != 4U) {
        return negative_response(kSidRoutineControl, kNrcIncorrectMessageLength);
    }

    if (session_manager_.current() != DiagnosticSession::Extended || !security_unlocked_) {
        return negative_response(kSidRoutineControl, kNrcConditionsNotCorrect);
    }

    const auto routine_id = static_cast<std::uint16_t>((request[2] << 8U) | request[3]);
    if (routine_id != kSelfTestRoutineId) {
        return negative_response(kSidRoutineControl, kNrcRequestOutOfRange);
    }

    constexpr std::uint8_t kStartRoutine = 0x01;
    constexpr std::uint8_t kStopRoutine = 0x02;
    constexpr std::uint8_t kRequestRoutineResults = 0x03;

    if (request[1] == kStartRoutine) {
        self_test_running_ = true;
        return ByteVector {static_cast<std::uint8_t>(kSidRoutineControl + 0x40U), request[1], request[2], request[3], 0x00};
    }

    if (request[1] == kStopRoutine) {
        self_test_running_ = false;
        return ByteVector {static_cast<std::uint8_t>(kSidRoutineControl + 0x40U), request[1], request[2], request[3], 0x00};
    }

    if (request[1] == kRequestRoutineResults) {
        const std::uint8_t status = self_test_running_ ? 0x01U : 0x00U;
        return ByteVector {static_cast<std::uint8_t>(kSidRoutineControl + 0x40U), request[1], request[2], request[3], status};
    }

    return negative_response(kSidRoutineControl, kNrcSubFunctionNotSupported);
}

ByteVector UdsDispatcher::negative_response(std::uint8_t sid, std::uint8_t nrc)
{
    return ByteVector {0x7F, sid, nrc};
}
