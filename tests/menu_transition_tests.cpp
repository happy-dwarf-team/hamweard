// The world-reset gate, after EU4 Menu Patch's tests/menu_transition_tests.cpp
// (MIT License, Copyright (c) 2026 VulonLok).
#include "map_snapshot.hpp"
#include "menu_transition.hpp"

#include <cstdio>
#include <array>
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
    {
        std::array<std::uint8_t, 2 * hamweard::adjacency_size> saved{}, current{};
        for (std::size_t i = 0; i < saved.size(); ++i) saved[i] = current[i] = static_cast<std::uint8_t>(i);
        check(hamweard::same_links(saved, current), "the same links");
        current[0xC] ^= 1;
        current[0x10 + hamweard::adjacency_size] ^= 1;
        check(hamweard::same_links(saved, current), "other costs are the same links");
        current[hamweard::adjacency_size] ^= 1;
        check(!hamweard::same_links(saved, current), "another neighbour");
        current[hamweard::adjacency_size] ^= 1;
        current[0x1C] ^= 1;
        check(!hamweard::same_links(saved, current), "another type");
        check(!hamweard::same_links(saved, std::span(current).first(hamweard::adjacency_size)), "another count");
    }
    hamweard::WorldResetGate gate;
    int app = 0, old = 0, menu = 0, other = 0, resets = 0;
    for (int cycle = 0; cycle < 1000; ++cycle) {
        bool outgoing_alive = true;
        gate.queue(&app, &menu);
        check(gate.pending(), "pending after queue");
        auto reset = [&] {
            check(!outgoing_alive, "reset while the outgoing idler is alive");
            // The history walk is skipped only while the gate is pending: the
            // reset's own walk has to run.
            check(!gate.pending(), "not pending during the reset");
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

    using hamweard::corruption_after_return;
    check(corruption_after_return(0, 0) == 0, "a clean world stays clean");
    check(corruption_after_return(1, 1) == 1, "the game's flag stays set");
    check(corruption_after_return(1, 2) == 2, "-nudge's reason comes back");
    check(corruption_after_return(0, 2) == 0, "nothing is set on a clean world");
    std::puts("menu transition tests passed");
}
