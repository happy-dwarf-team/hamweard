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
    {
        std::array<std::uint8_t, 2 * hamweard::adjacency_size> first{}, second{};
        std::array<std::uint8_t, 8> naval{};
        for (std::size_t i = 0; i < first.size(); ++i) first[i] = second[i] = static_cast<std::uint8_t>(i);
        for (std::size_t i = 0; i < naval.size(); ++i) naval[i] = static_cast<std::uint8_t>(i);
        const auto kept_first = first;
        const auto kept_naval = naval;
        const hamweard::MapView map{{std::span(first), std::span(second)}, std::span(naval)};

        hamweard::MapSnapshot snapshot;
        check(!snapshot.restore(map), "nothing to put back before a canal");
        snapshot.take({});
        check(!snapshot.taken(), "no map, no snapshot");
        snapshot.take(map);
        check(snapshot.taken(), "the map is kept");

        first[0xC] ^= 1;  // a cost
        naval[0] ^= 1;
        snapshot.take(map);  // a second canal must not replace the first picture
        check(snapshot.restore(map), "the same links go back");
        check(first == kept_first && naval == kept_naval, "costs and naval distances restored");

        first[0xC] ^= 1;
        naval[0] ^= 1;
        second[0] ^= 1;  // a neighbour the map did not have
        const auto changed = first;
        const auto changed_naval = naval;
        check(!snapshot.restore(map), "other links do not go back");
        check(first == changed && naval == changed_naval, "a failed restore writes nothing");
        second[0] ^= 1;

        check(!snapshot.restore({{std::span(first)}, std::span(naval)}), "another province count");
        check(!snapshot.restore({{std::span(first), std::span(second)}, std::span(naval).first(4)}), "another naval size");
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
    check(hamweard::enough_memory_for_return(hamweard::min_commit_for_return), "the limit is enough");
    check(!hamweard::enough_memory_for_return(hamweard::min_commit_for_return - 1), "below the limit the game restarts");
    const std::uint64_t plenty = hamweard::min_commit_for_return * 8;
    check(hamweard::restart_reason(false, false, plenty) == nullptr, "a plain return does not restart");
    check(hamweard::restart_reason(true, false, plenty) != nullptr, "multiplayer restarts");
    check(hamweard::restart_reason(false, true, plenty) != nullptr, "a Random New World map restarts, whatever the memory");
    check(hamweard::restart_reason(false, false, 0) != nullptr, "too little memory restarts");
    std::puts("menu transition tests passed");
}
