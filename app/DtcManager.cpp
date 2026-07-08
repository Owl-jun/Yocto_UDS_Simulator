#include "DtcManager.hpp"

namespace {

constexpr std::uint32_t kDtcEngineOverheat = 0x123456;
constexpr std::uint32_t kDtcBatteryVoltageLow = 0xABCDEF;
constexpr std::uint32_t kDtcSensorSignalLost = 0x010203;

constexpr std::uint8_t kDtcStatusTestFailed = 0x01;
constexpr std::uint8_t kDtcStatusConfirmed = 0x08;
constexpr std::uint8_t kDtcStatusTestFailedSinceLastClear = 0x20;
constexpr std::uint8_t kDtcStatusAvailabilityMask = 0xFF;

} // namespace

void DtcManager::reset_defaults()
{
    dtcs_ = {
        DtcEntry {kDtcEngineOverheat, kDtcStatusConfirmed},
        DtcEntry {kDtcBatteryVoltageLow, kDtcStatusTestFailed},
        DtcEntry {kDtcSensorSignalLost, static_cast<std::uint8_t>(kDtcStatusTestFailedSinceLastClear | kDtcStatusConfirmed)},
    };
}

std::vector<DtcEntry> DtcManager::read_by_status_mask(std::uint8_t status_mask) const
{
    std::vector<DtcEntry> matches;
    for (const auto& dtc : dtcs_) {
        if ((dtc.status & status_mask) != 0U) {
            matches.push_back(dtc);
        }
    }

    return matches;
}

void DtcManager::clear_all()
{
    dtcs_.clear();
}

std::uint8_t DtcManager::status_availability_mask() const
{
    return kDtcStatusAvailabilityMask;
}
