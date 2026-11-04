// From EU4 Menu Patch (https://github.com/YozoraTempest/EU4-Menu-Patch),
// MIT License, Copyright (c) 2026 VulonLok. See LICENSE.
#pragma once

#include <cstdint>

namespace hamweard {

// The world's PotentialCorruption after a return. The game sets it to 1 when
// the player enters single player and keeps it through ResetGame: it is what
// asks for a restart before multiplayer, so 1 stays. ResetGame turns every other
// non-zero reason into 1, and only -nudge's own reason (2) needs putting back.
constexpr std::uint32_t corruption_after_return(std::uint32_t after_reset, std::uint32_t before_return) noexcept {
    return after_reset == 1 && before_return > 1 ? before_return : after_reset;
}

// SetNextIdler hands the new idler over on a later frame. The world is reset
// only after that hand-over has destroyed the outgoing idler: once the menu is
// both the updating and the current idler and nothing is queued after it.
class WorldResetGate {
    void* app_ = nullptr;
    void* menu_ = nullptr;

public:
    void queue(void* app, void* menu) noexcept {
        app_ = app;
        menu_ = menu;
    }
    bool pending() const noexcept { return app_ != nullptr; }

    // Runs reset() once, when the hand-over is complete; disarms itself before
    // the call, so a reset that pumps messages or throws never runs twice.
    template <class Reset> bool run(void* app, void* updating, void* current, void* next, Reset&& reset) {
        if (!app_ || app != app_ || updating != menu_ || current != menu_ || next) return false;
        app_ = nullptr;
        menu_ = nullptr;
        reset();
        return true;
    }
};

}  // namespace hamweard
