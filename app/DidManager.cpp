#include "DidManager.hpp"

void DidManager::set(std::uint16_t did, const std::string& value)
{
    dids_[did] = ByteVector(value.begin(), value.end());
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
