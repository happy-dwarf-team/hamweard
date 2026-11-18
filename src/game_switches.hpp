#pragma once

#include <cstdint>

namespace hamweard {

// A switch of the game that only the console or a tutorial sets and the simulation reads,
// which nothing resets: a fresh process starts from its image value, and a return used to
// keep what the last campaign left in it (lab, 18 November 2026). It is kept once, before
// the first campaign, and put back at each return.
class KeptSwitch {
    std::int32_t value_ = 0;
    bool taken_ = false;

public:
    void take(std::int32_t value) noexcept {
        if (taken_) return;
        value_ = value;
        taken_ = true;
    }
    bool taken() const noexcept { return taken_; }
    std::int32_t value() const noexcept { return value_; }
    // Whether a return has to write the kept value back.
    bool differs(std::int32_t current) const noexcept { return taken_ && current != value_; }
};

}  // namespace hamweard
