// From EU4 Menu Patch (https://github.com/YozoraTempest/EU4-Menu-Patch),
// MIT License, Copyright (c) 2026 VulonLok. See LICENSE.
#pragma once

namespace menu_patch {

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

}  // namespace menu_patch
