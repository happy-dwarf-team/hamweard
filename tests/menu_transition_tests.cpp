// The world-reset gate, after EU4 Menu Patch's tests/menu_transition_tests.cpp
// (MIT License, Copyright (c) 2026 VulonLok).
#include "menu_transition.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {
void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::exit(1);
    }
}
}  // namespace

int main() {
    hamweard::WorldResetGate gate;
    int app = 0, old = 0, menu = 0, other = 0, resets = 0;
    for (int cycle = 0; cycle < 1000; ++cycle) {
        bool outgoing_alive = true;
        gate.queue(&app, &menu);
        check(gate.pending(), "pending after queue");
        auto reset = [&] {
            check(!outgoing_alive, "reset while the outgoing idler is alive");
            ++resets;
            // A reset can pump messages: it must never run twice.
            check(!gate.run(&app, &menu, &menu, nullptr, [] { std::exit(1); }), "nested reset");
        };
        check(!gate.run(&app, &old, &old, &menu, reset), "old idler updating, menu queued");
        check(!gate.run(&app, &menu, &old, &menu, reset), "menu updating before it is current");
        check(!gate.run(&other, &menu, &menu, nullptr, reset), "another application");
        check(!gate.run(&app, &menu, &menu, &other, reset), "another idler queued after the menu");
        outgoing_alive = false;
        check(gate.run(&app, &menu, &menu, nullptr, reset), "the reset once the menu owns the frame");
        check(!gate.run(&app, &menu, &menu, nullptr, reset), "a second reset");
        check(!gate.pending(), "not pending after the reset");
        check(resets == cycle + 1, "one reset per cycle");
    }
    gate.queue(&app, &menu);
    try {
        gate.run(&app, &menu, &menu, nullptr, [] { throw std::runtime_error("reset failed"); });
    } catch (const std::runtime_error&) {
    }
    check(!gate.run(&app, &menu, &menu, nullptr, [] { std::exit(1); }), "a failed reset is not retried");
    std::puts("menu transition tests passed");
}
