#include "UdsDispatcher.hpp"

#include "Logger.hpp"

#include <iomanip>
#include <sstream>

namespace {

constexpr std::uint8_t kSidDiagnosticSessionControl = 0x10;
constexpr std::uint8_t kSidEcuReset = 0x11;
constexpr std::uint8_t kSidClearDiagnosticInformation = 0x14;
constexpr std::uint8_t kSidReadDtcInformation = 0x19;
constexpr std::uint8_t kSidReadDataByIdentifier = 0x22;
constexpr std::uint8_t kSidSecurityAccess = 0x27;
constexpr std::uint8_t kSidWriteDataByIdentifier = 0x2E;
constexpr std::uint8_t kSidRoutineControl = 0x31;
constexpr std::uint8_t kSidNegativeResponse = 0x7F;
constexpr std::uint8_t kUnknownSid = 0x00;
constexpr std::uint8_t kPositiveResponseOffset = 0x40;

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
constexpr std::uint32_t kAllDtcGroup = 0xFFFFFF;
constexpr std::uint8_t kReportDtcByStatusMask = 0x02;
constexpr std::uint8_t kRequestSeed = 0x01;
constexpr std::uint8_t kSendKey = 0x02;
constexpr std::uint8_t kStartRoutine = 0x01;
constexpr std::uint8_t kStopRoutine = 0x02;
constexpr std::uint8_t kRequestRoutineResults = 0x03;
constexpr std::uint8_t kHardReset = 0x01;
constexpr std::uint8_t kSoftReset = 0x03;
constexpr std::uint8_t kRoutineOk = 0x00;
constexpr std::uint8_t kRoutineRunning = 0x01;
constexpr std::uint8_t kByteMask = 0xFF;
constexpr unsigned int kHighByteShift = 8U;
constexpr unsigned int kDtcHighByteShift = 16U;
constexpr unsigned int kDtcMiddleByteShift = 8U;

constexpr std::size_t kSidIndex = 0U;
constexpr std::size_t kSubFunctionIndex = 1U;
constexpr std::size_t kDidHighByteIndex = 1U;
constexpr std::size_t kDidLowByteIndex = 2U;
constexpr std::size_t kDtcGroupHighByteIndex = 1U;
constexpr std::size_t kDtcGroupMiddleByteIndex = 2U;
constexpr std::size_t kDtcGroupLowByteIndex = 3U;
constexpr std::size_t kDtcStatusMaskIndex = 2U;
constexpr std::size_t kRoutineIdHighByteIndex = 2U;
constexpr std::size_t kRoutineIdLowByteIndex = 3U;
constexpr std::size_t kSecurityKeyHighIndex = 2U;
constexpr std::size_t kSecurityKeyLowIndex = 3U;
constexpr std::size_t kWriteDidValueIndex = 3U;

constexpr std::size_t kSessionControlRequestLength = 2U;
constexpr std::size_t kEcuResetRequestLength = 2U;
constexpr std::size_t kClearDtcRequestLength = 4U;
constexpr std::size_t kReadDidRequestLength = 3U;
constexpr std::size_t kMinimumWriteDidRequestLength = 4U;
constexpr std::size_t kReadDtcRequestLength = 3U;
constexpr std::size_t kSecuritySeedRequestLength = 2U;
constexpr std::size_t kSecurityKeyRequestLength = 4U;
constexpr std::size_t kRoutineControlRequestLength = 4U;

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
    return static_cast<std::uint16_t>((request[kDidHighByteIndex] << kHighByteShift) | request[kDidLowByteIndex]);
}

} // namespace

UdsDispatcher::UdsDispatcher(DidManager& did_manager, DtcManager& dtc_manager, SessionManager& session_manager)
    : did_manager_(did_manager)
    , dtc_manager_(dtc_manager)
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
        return to_hex_line(negative_response(kUnknownSid, kNrcIncorrectMessageLength));
    }
}

ByteVector UdsDispatcher::dispatch(const ByteVector& request)
{
    if (request.empty()) {
        return negative_response(kUnknownSid, kNrcIncorrectMessageLength);
    }

    switch (request[kSidIndex]) {
    case kSidDiagnosticSessionControl:
        return handle_session_control(request);
    case kSidEcuReset:
        return handle_ecu_reset(request);
    case kSidClearDiagnosticInformation:
        return handle_clear_diagnostic_information(request);
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
        return negative_response(request[kSidIndex], kNrcServiceNotSupported);
    }
}

ByteVector UdsDispatcher::handle_session_control(const ByteVector& request)
{
    if (request.size() != kSessionControlRequestLength) {
        return negative_response(kSidDiagnosticSessionControl, kNrcIncorrectMessageLength);
    }

    if (!session_manager_.change(request[kSubFunctionIndex])) {
        return negative_response(kSidDiagnosticSessionControl, kNrcRequestOutOfRange);
    }

    if (session_manager_.current() == DiagnosticSession::Default) {
        security_seed_issued_ = false;
        security_unlocked_ = false;
        self_test_running_ = false;
    }

    Logger::info("Session changed to " + session_manager_.current_name());
    return ByteVector {static_cast<std::uint8_t>(kSidDiagnosticSessionControl + kPositiveResponseOffset), request[kSubFunctionIndex]};
}

ByteVector UdsDispatcher::handle_ecu_reset(const ByteVector& request)
{
    if (request.size() != kEcuResetRequestLength) {
        return negative_response(kSidEcuReset, kNrcIncorrectMessageLength);
    }

    if (request[kSubFunctionIndex] != kHardReset && request[kSubFunctionIndex] != kSoftReset) {
        return negative_response(kSidEcuReset, kNrcSubFunctionNotSupported);
    }

    session_manager_.reset();
    security_seed_issued_ = false;
    security_unlocked_ = false;
    self_test_running_ = false;
    Logger::info("ECU reset simulated, session returned to default");
    return ByteVector {static_cast<std::uint8_t>(kSidEcuReset + kPositiveResponseOffset), request[kSubFunctionIndex]};
}

ByteVector UdsDispatcher::handle_clear_diagnostic_information(const ByteVector& request)
{
    if (request.size() != kClearDtcRequestLength) {
        return negative_response(kSidClearDiagnosticInformation, kNrcIncorrectMessageLength);
    }

    if (session_manager_.current() != DiagnosticSession::Extended) {
        return negative_response(kSidClearDiagnosticInformation, kNrcConditionsNotCorrect);
    }

    const auto group_of_dtc = (static_cast<std::uint32_t>(request[kDtcGroupHighByteIndex]) << kDtcHighByteShift)
        | (static_cast<std::uint32_t>(request[kDtcGroupMiddleByteIndex]) << kDtcMiddleByteShift)
        | request[kDtcGroupLowByteIndex];
    if (group_of_dtc != kAllDtcGroup) {
        return negative_response(kSidClearDiagnosticInformation, kNrcRequestOutOfRange);
    }

    dtc_manager_.clear_all();
    return ByteVector {static_cast<std::uint8_t>(kSidClearDiagnosticInformation + kPositiveResponseOffset)};
}

ByteVector UdsDispatcher::handle_read_did(const ByteVector& request)
{
    if (request.size() != kReadDidRequestLength) {
        return negative_response(kSidReadDataByIdentifier, kNrcIncorrectMessageLength);
    }

    const auto did = read_did_from_request(request);
    const auto value = did_manager_.read(did);
    if (!value) {
        return negative_response(kSidReadDataByIdentifier, kNrcRequestOutOfRange);
    }

    ByteVector response {
        static_cast<std::uint8_t>(kSidReadDataByIdentifier + kPositiveResponseOffset),
        request[kDidHighByteIndex],
        request[kDidLowByteIndex],
    };
    response.insert(response.end(), value->begin(), value->end());
    return response;
}

ByteVector UdsDispatcher::handle_write_did(const ByteVector& request)
{
    if (request.size() < kMinimumWriteDidRequestLength) {
        return negative_response(kSidWriteDataByIdentifier, kNrcIncorrectMessageLength);
    }

    if (!session_manager_.is_write_allowed()) {
        return negative_response(kSidWriteDataByIdentifier, kNrcConditionsNotCorrect);
    }

    const auto did = read_did_from_request(request);
    const ByteVector value(request.begin() + kWriteDidValueIndex, request.end());
    if (!did_manager_.write(did, value)) {
        return negative_response(kSidWriteDataByIdentifier, kNrcRequestOutOfRange);
    }

    return ByteVector {
        static_cast<std::uint8_t>(kSidWriteDataByIdentifier + kPositiveResponseOffset),
        request[kDidHighByteIndex],
        request[kDidLowByteIndex],
    };
}

ByteVector UdsDispatcher::handle_read_dtc_information(const ByteVector& request)
{
    if (request.size() != kReadDtcRequestLength) {
        return negative_response(kSidReadDtcInformation, kNrcIncorrectMessageLength);
    }

    if (request[kSubFunctionIndex] != kReportDtcByStatusMask) {
        return negative_response(kSidReadDtcInformation, kNrcSubFunctionNotSupported);
    }

    ByteVector response {
        static_cast<std::uint8_t>(kSidReadDtcInformation + kPositiveResponseOffset),
        request[kSubFunctionIndex],
        dtc_manager_.status_availability_mask(),
    };
    const auto dtcs = dtc_manager_.read_by_status_mask(request[kDtcStatusMaskIndex]);
    for (const auto& dtc : dtcs) {
        response.push_back(static_cast<std::uint8_t>((dtc.code >> kDtcHighByteShift) & kByteMask));
        response.push_back(static_cast<std::uint8_t>((dtc.code >> kDtcMiddleByteShift) & kByteMask));
        response.push_back(static_cast<std::uint8_t>(dtc.code & kByteMask));
        response.push_back(dtc.status);
    }

    return response;
}

ByteVector UdsDispatcher::handle_security_access(const ByteVector& request)
{
    if (request.size() != kSecuritySeedRequestLength && request.size() != kSecurityKeyRequestLength) {
        return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
    }

    if (session_manager_.current() != DiagnosticSession::Extended) {
        return negative_response(kSidSecurityAccess, kNrcConditionsNotCorrect);
    }

    if (request[kSubFunctionIndex] == kRequestSeed) {
        if (request.size() != kSecuritySeedRequestLength) {
            return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
        }

        security_seed_issued_ = true;
        security_unlocked_ = false;
        return ByteVector {
            static_cast<std::uint8_t>(kSidSecurityAccess + kPositiveResponseOffset),
            request[kSubFunctionIndex],
            kSecuritySeedHigh,
            kSecuritySeedLow,
        };
    }

    if (request[kSubFunctionIndex] == kSendKey) {
        if (request.size() != kSecurityKeyRequestLength) {
            return negative_response(kSidSecurityAccess, kNrcIncorrectMessageLength);
        }

        if (!security_seed_issued_) {
            return negative_response(kSidSecurityAccess, kNrcRequestSequenceError);
        }

        if (request[kSecurityKeyHighIndex] != kSecurityKeyHigh || request[kSecurityKeyLowIndex] != kSecurityKeyLow) {
            security_unlocked_ = false;
            return negative_response(kSidSecurityAccess, kNrcInvalidKey);
        }

        security_unlocked_ = true;
        return ByteVector {static_cast<std::uint8_t>(kSidSecurityAccess + kPositiveResponseOffset), request[kSubFunctionIndex]};
    }

    return negative_response(kSidSecurityAccess, kNrcSubFunctionNotSupported);
}

ByteVector UdsDispatcher::handle_routine_control(const ByteVector& request)
{
    if (request.size() != kRoutineControlRequestLength) {
        return negative_response(kSidRoutineControl, kNrcIncorrectMessageLength);
    }

    if (session_manager_.current() != DiagnosticSession::Extended || !security_unlocked_) {
        return negative_response(kSidRoutineControl, kNrcConditionsNotCorrect);
    }

    const auto routine_id = static_cast<std::uint16_t>((request[kRoutineIdHighByteIndex] << kHighByteShift) | request[kRoutineIdLowByteIndex]);
    if (routine_id != kSelfTestRoutineId) {
        return negative_response(kSidRoutineControl, kNrcRequestOutOfRange);
    }

    if (request[kSubFunctionIndex] == kStartRoutine) {
        self_test_running_ = true;
        return ByteVector {
            static_cast<std::uint8_t>(kSidRoutineControl + kPositiveResponseOffset),
            request[kSubFunctionIndex],
            request[kRoutineIdHighByteIndex],
            request[kRoutineIdLowByteIndex],
            kRoutineOk,
        };
    }

    if (request[kSubFunctionIndex] == kStopRoutine) {
        self_test_running_ = false;
        return ByteVector {
            static_cast<std::uint8_t>(kSidRoutineControl + kPositiveResponseOffset),
            request[kSubFunctionIndex],
            request[kRoutineIdHighByteIndex],
            request[kRoutineIdLowByteIndex],
            kRoutineOk,
        };
    }

    if (request[kSubFunctionIndex] == kRequestRoutineResults) {
        const std::uint8_t status = self_test_running_ ? kRoutineRunning : kRoutineOk;
        return ByteVector {
            static_cast<std::uint8_t>(kSidRoutineControl + kPositiveResponseOffset),
            request[kSubFunctionIndex],
            request[kRoutineIdHighByteIndex],
            request[kRoutineIdLowByteIndex],
            status,
        };
    }

    return negative_response(kSidRoutineControl, kNrcSubFunctionNotSupported);
}

ByteVector UdsDispatcher::negative_response(std::uint8_t sid, std::uint8_t nrc)
{
    return ByteVector {kSidNegativeResponse, sid, nrc};
}
