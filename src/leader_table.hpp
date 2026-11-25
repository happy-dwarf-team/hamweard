// The leaders' ID table. The global ID tables are an array of 4711 pointers, one per object
// type, each to a std::unordered_map<unsigned, object*> (MSVC: its list's sentinel node at +8;
// a node is next, prev, then the key at +0x10 and the object at +0x18), as the game's lookup
// of an object by its ID reads it. Leaders are type 0x31.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace hamweard {

inline constexpr std::uint32_t leader_id_type = 0x31;
inline constexpr std::ptrdiff_t id_map_list_head = 0x8;
inline constexpr std::ptrdiff_t id_node_object = 0x18;

// The objects in one type's map, copied out: deleting one erases its node.
inline std::vector<std::uint8_t*> id_table_objects(const std::uintptr_t* tables, std::uint32_t type) {
    std::vector<std::uint8_t*> objects;
    const auto* map = reinterpret_cast<const std::uint8_t*>(tables[type]);
    if (!map) return objects;
    const std::uint8_t* sentinel = nullptr;
    std::memcpy(&sentinel, map + id_map_list_head, sizeof sentinel);
    if (!sentinel) return objects;
    const std::uint8_t* node = nullptr;
    std::memcpy(&node, sentinel, sizeof node);
    while (node && node != sentinel) {
        std::uint8_t* object = nullptr;
        std::memcpy(&object, node + id_node_object, sizeof object);
        if (object) objects.push_back(object);
        std::memcpy(&node, node, sizeof node);
    }
    return objects;
}

}  // namespace hamweard
