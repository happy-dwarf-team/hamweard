// A rebel faction (CRebelFaction, 0x190 bytes, ID type 0x32) of the old campaign. Its destructor
// reaches into the world: it deletes the units on its unit list (through the game state's deletion
// of a unit), takes each leader on its leader list from its country and deletes it, gives its
// occupied provinces back to their controllers, recalculates the revolts of its possible provinces,
// and, unless its being-deleted flag is set, takes itself from its home country and its supporter.
// Detached, it frees only its own strings and arrays.
//
// The old game state's destructor runs the game state's clear, whose rebel_stuff_clear runs the
// ID's destroy on every faction: that erases the ID and sets the ID's type to 0xFFFE, but deletes
// nothing, and the registered flag stays set. A later destroy, as the faction's own destructor
// ends with, would then index the ID tables with 0xFFFE, past their 4711 entries. So a faction is
// deleted only once its ID is gone, with the flag cleared first.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hamweard {

inline constexpr std::uint32_t rebel_faction_id_type = 0x32;
inline constexpr std::ptrdiff_t reference_id_type = 0x10;     // the object's ID: its type
inline constexpr std::ptrdiff_t reference_registered = 0x18;  // bool: the destroy erases the ID
inline constexpr std::uint32_t destroyed_id_type = 0xFFFE;    // set by the destroy
inline constexpr std::ptrdiff_t faction_units = 0xE8;              // a CList of unit IDs
inline constexpr std::ptrdiff_t faction_occupied = 0x118;          // an int array: the occupied provinces
inline constexpr std::ptrdiff_t faction_leaders = 0x150;           // a CList of CLeader*: its leaders
inline constexpr std::ptrdiff_t faction_revolt_provinces = 0x168;  // a CList of int: the possible revolt provinces
inline constexpr std::ptrdiff_t faction_in_delete = 0x188;         // bool: being deleted

// Whether the destroy has erased the object's ID.
inline bool id_destroyed(const std::uint8_t* object) {
    std::uint32_t type = 0;
    std::memcpy(&type, object + reference_id_type, sizeof type);
    return type == destroyed_id_type;
}

// A CList: head, tail and count (+0, +8, +0x10); a node of 0x20 bytes has its next at +0x10.
// Frees every node with free_node and leaves the list empty. Returns the nodes freed.
template <class FreeNode> std::size_t free_list_nodes(std::uint8_t* list, FreeNode&& free_node) {
    std::uint8_t* node = nullptr;
    std::memcpy(&node, list, sizeof node);
    std::size_t n = 0;
    while (node) {
        std::uint8_t* next = nullptr;
        std::memcpy(&next, node + 0x10, sizeof next);
        free_node(node);
        node = next;
        ++n;
    }
    std::memset(list, 0, 0x14);
    return n;
}

// Empties what the faction's destructor would follow into the world: the three lists (their nodes freed,
// not what they hold: the units are gone with the world, the leaders stay in the leader table for
// Hamweard's own clean-up), the occupied provinces (end = begin, the buffer stays for the array's
// destructor), and sets the being-deleted flag, so that the destructor does not look for its home
// country. For a faction whose ID the destroy has erased, it also clears the registered flag, so
// that the destructor's own destroy does nothing.
template <class FreeNode> void detach_rebel_faction(std::uint8_t* faction, FreeNode&& free_node) {
    free_list_nodes(faction + faction_units, free_node);
    free_list_nodes(faction + faction_leaders, free_node);
    free_list_nodes(faction + faction_revolt_provinces, free_node);
    std::memcpy(faction + faction_occupied + 8, faction + faction_occupied, 8);
    faction[faction_in_delete] = 1;
    if (id_destroyed(faction)) faction[reference_registered] = 0;
}

}  // namespace hamweard
