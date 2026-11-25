// The AI's strength factor caches. Their list is a CPdxArray of cache pointers: vtable, data,
// capacity, size. Each thread that asks the cache lookup gets one cache of 0xC8 bytes, kept for
// the process: a vtable and three unordered_maps keyed by CUnit*, CProvince* and CRebelFaction*.
// The AI's daily update clears the three maps of every cache at the end of each AI day,
// and nothing else does, so what the last campaign put in after its last AI day stays, keyed
// by addresses its world no longer owns.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace hamweard {

inline constexpr std::ptrdiff_t cache_list_data = 0x8;
inline constexpr std::ptrdiff_t cache_list_size = 0x14;
inline constexpr std::ptrdiff_t strength_cache_maps[3] = {0x8, 0x48, 0x88};
inline constexpr std::ptrdiff_t hash_map_size = 0x10;  // MSVC's unordered_map: its list's size

inline std::span<std::uint8_t* const> strength_caches(const std::uint8_t* list) noexcept {
    std::uint8_t* const* data = nullptr;
    std::int32_t size = 0;
    std::memcpy(&data, list + cache_list_data, sizeof data);
    std::memcpy(&size, list + cache_list_size, sizeof size);
    if (!data || size <= 0) return {};
    return {data, static_cast<std::size_t>(size)};
}

inline std::uint64_t strength_cache_entries(const std::uint8_t* cache) noexcept {
    std::uint64_t total = 0;
    for (std::ptrdiff_t map : strength_cache_maps) {
        std::uint64_t size = 0;
        std::memcpy(&size, cache + map + hash_map_size, sizeof size);
        total += size;
    }
    return total;
}

}  // namespace hamweard
