// The world-reset gate, after EU4 Menu Patch's tests/menu_transition_tests.cpp
// (MIT License, Copyright (c) 2026 VulonLok).
#include "build_layout.hpp"
#include "estates.hpp"
#include "game_switches.hpp"
#include "leader_table.hpp"
#include "map_extras.hpp"
#include "map_snapshot.hpp"
#include "menu_transition.hpp"
#include "rebel_factions.hpp"
#include "strength_caches.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::exit(1);
    }
}
}  // namespace

int main() {
    check(hamweard::multiplayer_needs_restart(true, 1), "a played process restarts first");
    check(hamweard::multiplayer_needs_restart(true, 2), "so does -nudge's reason");
    check(!hamweard::multiplayer_needs_restart(true, 0), "nothing played, nothing to ask");
    check(!hamweard::multiplayer_needs_restart(false, 1), "no return, as without the plugin");
    check(hamweard::multiplayer_availability(0, true, 1) == hamweard::multiplayer_must_restart,
          "without a Paradox session the tooltip still gives the reason");
    check(hamweard::multiplayer_availability(5, true, 1) == 5, "the game's own answer stays");
    check(hamweard::multiplayer_availability(4, true, 1) == 4, "so does another reason");
    check(hamweard::multiplayer_availability(0, false, 1) == 0, "a fresh process stays available");
    check(hamweard::multiplayer_availability(0, true, 0) == 0, "nothing played, nothing to say");
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
    check(hamweard::restart_reason(true, false, plenty) != nullptr, "Shift held restarts");
    check(std::string_view(hamweard::restart_reason(true, true, 0)) == "with Shift held", "and says so first");
    check(hamweard::restart_reason(false, true, plenty) != nullptr, "multiplayer restarts");
    check(hamweard::restart_reason(false, false, 0) != nullptr, "too little memory restarts");
    check(std::string_view(hamweard::restart_reason(false, true, 0)) == "in a multiplayer session",
          "multiplayer is named before the memory");

    // hamweard#88: a reset on the frontend gets the validation of a fresh start.
    {
        int multiplayer_asked = 0, idler_asked = 0;
        auto needs = [&](bool keep_history, bool reloading, bool multiplayer, bool idler_validates) {
            return hamweard::reset_needs_validation(
                keep_history, reloading, [&] { ++multiplayer_asked; return multiplayer; },
                [&] { ++idler_asked; return idler_validates; });
        };
        check(needs(false, false, false, false), "a frontend reset that reloads the history is validated");
        check(!needs(true, false, false, false), "a reset that keeps the history is left alone");
        check(!needs(false, true, false, false), "Hamweard's own return validates by itself");
        check(!needs(false, false, false, true), "an idler that validates is left alone");
        multiplayer_asked = idler_asked = 0;
        check(!needs(false, false, true, false), "a multiplayer reset runs as stock");
        check(multiplayer_asked == 1 && idler_asked == 0, "a multiplayer reset never asks the idler");
        multiplayer_asked = idler_asked = 0;
        check(!needs(true, false, true, false) && multiplayer_asked == 0, "a kept history asks nothing");
    }

    using hamweard::BuildLayout;
    check(hamweard::build_layout("eu4-1.37.5-steam-win64") == BuildLayout::steam, "the Steam build");
    check(hamweard::build_layout("eu4-1.37.5.2-gog-win64") == BuildLayout::gog, "GOG 1.37.5.2");
    check(hamweard::build_layout("eu4-1.37.5.0-gog-win64") == BuildLayout::gog, "GOG 1.37.5.0");
    check(hamweard::build_layout("eu4-1.37.5-msstore-win64") == BuildLayout::unknown, "another store has no layout");
    check(hamweard::build_layout("") == BuildLayout::unknown, "no build has no layout");
    {
        hamweard::KeptSwitch dice;
        check(!dice.differs(3), "nothing kept, nothing to put back");
        dice.take(-1);
        dice.take(3);  // a later frontend tick: the first menu's value stays
        check(dice.taken() && dice.value() == -1, "the first main menu's value is kept");
        check(!dice.differs(-1), "an unchanged switch is left alone");
        check(dice.differs(3), "the console's fixed dice is put back");
    }
    {
        std::array<std::uint8_t, 0xC8> first{}, second{};
        auto put = [](std::uint8_t* at, auto value) { std::memcpy(at, &value, sizeof value); };
        put(first.data() + 0x8 + 0x10, std::uint64_t{3});    // units
        put(first.data() + 0x48 + 0x10, std::uint64_t{2});   // provinces
        put(second.data() + 0x88 + 0x10, std::uint64_t{1});  // rebel factions
        std::array<std::uint8_t*, 2> caches{first.data(), second.data()};
        std::array<std::uint8_t, 0x18> list{};
        check(hamweard::strength_caches(list.data()).empty(), "no thread asked, no cache");
        put(list.data() + 0x8, caches.data());
        put(list.data() + 0x14, std::int32_t{2});
        const auto found = hamweard::strength_caches(list.data());
        check(found.size() == 2 && found[1] == second.data(), "one cache per thread");
        check(hamweard::strength_cache_entries(found[0]) == 5 && hamweard::strength_cache_entries(found[1]) == 1,
              "the entries of the three maps");
    }
    {
        // a type's map: the sentinel node and two nodes in a ring, the objects at +0x18
        struct Node {
            Node* next;
            Node* prev;
            std::uint64_t key;
            std::uint8_t* object;
        };
        std::uint8_t a = 0, b = 0;
        Node sentinel{}, first{}, second{};
        sentinel.next = &first;
        first.next = &second;
        second.next = &sentinel;
        first.object = &a;
        second.object = &b;
        std::array<std::uint8_t, 0x40> map{};
        Node* head = &sentinel;
        std::memcpy(map.data() + 0x8, &head, sizeof head);
        std::vector<std::uintptr_t> tables(0x40);
        check(hamweard::id_table_objects(tables.data(), hamweard::leader_id_type).empty(), "no map, no leaders");
        tables[hamweard::leader_id_type] = reinterpret_cast<std::uintptr_t>(map.data());
        const auto leaders = hamweard::id_table_objects(tables.data(), hamweard::leader_id_type);
        check(leaders.size() == 2 && leaders[0] == &a && leaders[1] == &b, "every object of the type's map");
        sentinel.next = &sentinel;
        check(hamweard::id_table_objects(tables.data(), hamweard::leader_id_type).empty(), "an empty map");
    }
    {
        // two estates; the first holds two influence modifiers, the second one loyalty modifier
        std::array<std::uint8_t, 2 * 0xA8> estates{};
        std::array<std::uint8_t*, 2> influence{};
        std::array<std::uint8_t*, 1> loyalty{};
        std::uint8_t x = 0, y = 0, z = 0;
        influence = {&x, &y};
        loyalty = {&z};
        auto put = [](std::uint8_t* at, auto value) { std::memcpy(at, &value, sizeof value); };
        put(estates.data() + 0x58, influence.data());
        put(estates.data() + 0x58 + 8, influence.data() + 2);
        put(estates.data() + 0xA8 + 0x70, loyalty.data());
        put(estates.data() + 0xA8 + 0x70 + 8, loyalty.data() + 1);
        std::vector<std::uint8_t> country(0x21B0 + 0x18);
        check(hamweard::take_estate_values(country.data()).empty(), "a country without estates");
        put(country.data() + 0x21B0, estates.data());
        put(country.data() + 0x21B0 + 8, estates.data() + estates.size());
        const auto values = hamweard::take_estate_values(country.data());
        check(values.size() == 3 && values[0] == &x && values[1] == &y && values[2] == &z, "every estate's modifiers");
        check(hamweard::take_estate_values(country.data()).empty(), "the arrays are left empty");
    }
    {
        // a rebel faction with two units, one leader and no possible revolt provinces
        std::vector<std::uint8_t> faction(0x190);
        std::array<std::uint8_t, 0x20> unit_a{}, unit_b{}, leader{};
        std::array<int, 3> occupied{4, 5, 6};
        auto put = [](std::uint8_t* at, auto value) { std::memcpy(at, &value, sizeof value); };
        put(unit_a.data() + 0x10, unit_b.data());
        put(faction.data() + hamweard::faction_units, unit_a.data());
        put(faction.data() + hamweard::faction_units + 8, unit_b.data());
        put(faction.data() + hamweard::faction_units + 0x10, 2);
        put(faction.data() + hamweard::faction_leaders, leader.data());
        put(faction.data() + hamweard::faction_occupied, occupied.data());
        put(faction.data() + hamweard::faction_occupied + 8, occupied.data() + 3);
        put(faction.data() + hamweard::reference_id_type, hamweard::rebel_faction_id_type);
        faction[hamweard::reference_registered] = 1;
        std::vector<std::uint8_t*> freed;
        const auto free_node = [&](std::uint8_t* node) { freed.push_back(node); };

        hamweard::detach_rebel_faction(faction.data(), free_node);
        check(freed.size() == 3 && freed[0] == unit_a.data() && freed[1] == unit_b.data() && freed[2] == leader.data(),
              "the lists' nodes are freed");
        const std::vector<std::uint8_t> zeros(0x14);
        check(std::memcmp(faction.data() + hamweard::faction_units, zeros.data(), 0x14) == 0, "the units list is empty");
        check(std::memcmp(faction.data() + hamweard::faction_leaders, zeros.data(), 0x14) == 0, "so is the leaders list");
        check(std::memcmp(faction.data() + hamweard::faction_occupied + 8, faction.data() + hamweard::faction_occupied, 8) == 0,
              "the occupied provinces are empty, their buffer kept");
        check(faction[hamweard::faction_in_delete] == 1, "the home country is not looked for");
        check(faction[hamweard::reference_registered] == 1, "a registered ID stays for the destructor to erase");
        check(!hamweard::id_destroyed(faction.data()), "the ID is still there");

        put(faction.data() + hamweard::reference_id_type, hamweard::destroyed_id_type);
        check(hamweard::id_destroyed(faction.data()), "Destroy has erased it");
        freed.clear();
        hamweard::detach_rebel_faction(faction.data(), free_node);
        check(freed.empty(), "nothing is freed twice");
        check(faction[hamweard::reference_registered] == 0, "the destructor's own Destroy does nothing");
    }
    {
        // the map's extras: one field and two arrays, as Random New World leaves them
        auto put = [](std::uint8_t* at, auto value) { std::memcpy(at, &value, sizeof value); };
        auto set_array = [&](std::uint8_t* array, std::uint8_t* begin, std::size_t size, std::size_t capacity) {
            put(array + hamweard::array_begin, begin);
            put(array + hamweard::array_end, begin + size);
            put(array + hamweard::array_capacity, begin + capacity);
        };
        std::array<std::uint8_t, 2> flags{0x40, 0x00};
        std::array<std::uint8_t, 0x18> short_array{}, long_array{};
        std::array<std::uint8_t, 8> short_buffer{1, 2, 3, 4, 5, 6, 7, 8};
        std::array<std::uint8_t, 4> long_buffer{9, 9, 9, 9};
        set_array(short_array.data(), short_buffer.data(), 8, 8);
        set_array(long_array.data(), long_buffer.data(), 4, 4);
        hamweard::MapExtrasView view;
        view.fields.emplace_back(flags.data(), flags.size());
        view.arrays = {short_array.data(), long_array.data()};
        hamweard::MapExtrasSnapshot kept;
        kept.take(view);
        check(kept.taken() && kept.differences(view) == 0, "the snapshot is the map");

        // the change: a river flag set, the first array emptied, the second given other values
        flags[0] = 0x50;
        set_array(short_array.data(), short_buffer.data(), 0, 8);
        long_buffer = {7, 7, 7, 7};
        check(kept.differences(view) == 3, "three parts differ");
        std::vector<std::vector<std::uint8_t>> blocks;
        std::vector<void*> freed;
        auto alloc = [&](std::size_t size) -> void* { return blocks.emplace_back(size).data(); };
        auto free = [&](void* memory) { freed.push_back(memory); };
        check(kept.restore(view, alloc, free), "it goes back");
        check(kept.differences(view) == 0 && flags[0] == 0x40, "every part as kept");
        check(blocks.empty() && freed.empty(), "in place, without a new block");

        // an array kept longer than its room now gets a new block, the old one freed
        hamweard::MapExtrasView one;
        one.arrays = {long_array.data()};
        std::array<std::uint8_t, 0x18> grown{};
        std::array<std::uint8_t, 12> grown_buffer{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        set_array(grown.data(), grown_buffer.data(), 12, 12);
        hamweard::MapExtrasView grown_view;
        grown_view.arrays = {grown.data()};
        hamweard::MapExtrasSnapshot kept_long;
        kept_long.take(grown_view);
        check(kept_long.restore(one, alloc, free), "a longer array goes back");
        check(blocks.size() == 1 && freed.size() == 1 && freed[0] == long_buffer.data(), "in a new block");
        check(kept_long.differences(one) == 0, "with every byte");

        // a block the game's allocator gives a header to is not reallocated
        std::vector<std::uint8_t> big(hamweard::big_block), big_saved(hamweard::big_block + 4, 1);
        std::array<std::uint8_t, 0x18> big_array{}, big_kept_array{};
        set_array(big_array.data(), big.data(), big.size(), big.size());
        set_array(big_kept_array.data(), big_saved.data(), big_saved.size(), big_saved.size());
        hamweard::MapExtrasView big_view, big_kept_view;
        big_view.arrays = {big_array.data()};
        big_kept_view.arrays = {big_kept_array.data()};
        hamweard::MapExtrasSnapshot kept_big;
        kept_big.take(big_kept_view);
        const std::size_t before = blocks.size();
        check(!kept_big.restore(big_view, alloc, free), "a big array cannot grow: the game restarts");
        check(blocks.size() == before && big[0] == 0, "and nothing was written");

        hamweard::MapExtrasView other;
        check(!kept.restore(other, alloc, free), "another map does not take the snapshot");
        check(kept.differences(other) == SIZE_MAX, "and counts as all different");
    }
    std::puts("menu transition tests passed");
}
