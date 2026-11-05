#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace hamweard {

// One SAdjacency of a province template: 0x24 bytes in both game builds.
inline constexpr std::size_t adjacency_size = 0x24;

// Whether a province's neighbours after a world reset are the ones saved before
// the first canal, only with other costs. The fields that name a link are the
// neighbour (+0), the source (+4), the passage (+8) and the type (+0x1C).
// CalculateNeighborPaths rewrites the two costs (+0xC, +0x10) and nothing else.
inline bool same_links(std::span<const std::uint8_t> saved, std::span<const std::uint8_t> current) noexcept {
    if (saved.size() != current.size() || saved.size() % adjacency_size != 0) return false;
    for (std::size_t i = 0; i < saved.size(); i += adjacency_size) {
        if (std::memcmp(saved.data() + i, current.data() + i, 0xC) != 0) return false;
        if (std::memcmp(saved.data() + i + 0x1C, current.data() + i + 0x1C, 4) != 0) return false;
    }
    return true;
}

}  // namespace hamweard
