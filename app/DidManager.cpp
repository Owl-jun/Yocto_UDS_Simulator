#include "DidManager.hpp"

void DidManager::set(std::uint16_t did, const std::string& value)
{
    dids_[did] = ByteVector(value.begin(), value.end());
}

void DidManager::reset_defaults(const std::string& vin)
{
    dids_.clear();
    set(0xF190, vin);
    set(0xF187, "SW-1.0.0");
    set(0xF188, "HW-QEMU-X86_64");
    set(0xF18C, "SIM-0001");
}

std::optional<ByteVector> DidManager::read(std::uint16_t did) const
{
    const auto it = dids_.find(did);
    if (it == dids_.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool DidManager::write(std::uint16_t did, const ByteVector& value)
{
    const auto it = dids_.find(did);
    if (it == dids_.end()) {
        return false;
    }

    it->second = value;
    return true;
}
