#include "UdsDispatcher.hpp"

#include "Logger.hpp"

#include <iomanip>
#include <sstream>

namespace {

constexpr std::uint8_t kSidDiagnosticSessionControl = 0x10;
constexpr std::uint8_t kSidReadDataByIdentifier = 0x22;
constexpr std::uint8_t kSidWriteDataByIdentifier = 0x2E;

constexpr std::uint8_t kNrcIncorrectMessageLength = 0x13;
constexpr std::uint8_t kNrcConditionsNotCorrect = 0x22;
constexpr std::uint8_t kNrcRequestOutOfRange = 0x31;
constexpr std::uint8_t kNrcServiceNotSupported = 0x11;

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
    case kSidReadDataByIdentifier:
        return handle_read_did(request);
    case kSidWriteDataByIdentifier:
        return handle_write_did(request);
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

    Logger::info("Session changed to " + session_manager_.current_name());
    return ByteVector {static_cast<std::uint8_t>(kSidDiagnosticSessionControl + 0x40U), request[1]};
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

ByteVector UdsDispatcher::negative_response(std::uint8_t sid, std::uint8_t nrc)
{
    return ByteVector {0x7F, sid, nrc};
}
