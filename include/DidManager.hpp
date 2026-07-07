#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

using ByteVector = std::vector<std::uint8_t>;

class DidManager {
public:
    void set(std::uint16_t did, const std::string& value);
    std::optional<ByteVector> read(std::uint16_t did) const;
    bool write(std::uint16_t did, const ByteVector& value);

private:
    std::map<std::uint16_t, ByteVector> dids_;
};
