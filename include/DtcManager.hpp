#pragma once

#include <cstdint>
#include <vector>

struct DtcEntry {
    std::uint32_t code;
    std::uint8_t status;
};

class DtcManager {
public:
    void reset_defaults();
    std::vector<DtcEntry> read_by_status_mask(std::uint8_t status_mask) const;
    void clear_all();
    std::uint8_t status_availability_mask() const;

private:
    std::vector<DtcEntry> dtcs_;
};
