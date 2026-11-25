// The parts of the map and its load-once databases that the game's own restore of a Random New
// World map, or its removal of a canal, leaves different from the map the process loaded
// (hamweard#118): kept before the first change and put back after the reset.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace hamweard {

// The game's arrays (std::vector's layout): begin, end and capacity at +0, +8, +0x10.
inline constexpr std::ptrdiff_t array_begin = 0, array_end = 8, array_capacity = 0x10;
// MSVC's allocator puts a header before blocks of this size and above, which a plain operator
// delete would not find: Hamweard reallocates only below it.
inline constexpr std::size_t big_block = 0x1000;

inline std::uint8_t* array_pointer(const std::uint8_t* array, std::ptrdiff_t at) {
    std::uint8_t* p = nullptr;
    std::memcpy(&p, array + at, sizeof p);
    return p;
}

inline void set_array_pointer(std::uint8_t* array, std::ptrdiff_t at, std::uint8_t* p) {
    std::memcpy(array + at, &p, sizeof p);
}

inline std::span<std::uint8_t> array_bytes(const std::uint8_t* array) {
    std::uint8_t* begin = array_pointer(array, array_begin);
    std::uint8_t* end = array_pointer(array, array_end);
    if (!begin || end < begin) return {};
    return {begin, static_cast<std::size_t>(end - begin)};
}

// Whether put_array can write `size` bytes into the array: in place, or in a new block.
inline bool array_takes(const std::uint8_t* array, std::size_t size) {
    std::uint8_t* begin = array_pointer(array, array_begin);
    std::uint8_t* capacity = array_pointer(array, array_capacity);
    const std::size_t room = begin && capacity >= begin ? static_cast<std::size_t>(capacity - begin) : 0;
    if (size <= room) return true;
    return size < big_block && room < big_block;
}

// Writes `saved` into the array: in place when it fits, otherwise into a block from `alloc`,
// with the old one given to `free`. Call array_takes first.
template <class Alloc, class Free>
void put_array(std::uint8_t* array, std::span<const std::uint8_t> saved, Alloc&& alloc, Free&& free) {
    std::uint8_t* begin = array_pointer(array, array_begin);
    std::uint8_t* capacity = array_pointer(array, array_capacity);
    const std::size_t room = begin && capacity >= begin ? static_cast<std::size_t>(capacity - begin) : 0;
    if (saved.size() > room) {
        auto* block = static_cast<std::uint8_t*>(alloc(saved.size()));
        if (begin) free(begin);
        begin = block;
        set_array_pointer(array, array_capacity, block + saved.size());
    }
    if (!saved.empty()) std::memcpy(begin, saved.data(), saved.size());
    set_array_pointer(array, array_begin, begin);
    set_array_pointer(array, array_end, begin + saved.size());
}

// Where the kept parts are in the live map: fixed fields (spans) and arrays (the address of
// each array), in the same order at every call.
struct MapExtrasView {
    std::vector<std::span<std::uint8_t>> fields;
    std::vector<std::uint8_t*> arrays;
};

class MapExtrasSnapshot {
    std::vector<std::vector<std::uint8_t>> fields_, arrays_;
    bool taken_ = false;

public:
    bool taken() const noexcept { return taken_; }

    // Only the first call counts: later changes build on the first.
    void take(const MapExtrasView& map) {
        if (taken_) return;
        fields_.reserve(map.fields.size());
        for (const auto& field : map.fields) fields_.emplace_back(field.begin(), field.end());
        arrays_.reserve(map.arrays.size());
        for (const auto* array : map.arrays) {
            const auto bytes = array_bytes(array);
            arrays_.emplace_back(bytes.begin(), bytes.end());
        }
        taken_ = true;
    }

    // How many fields and arrays differ from the snapshot now.
    std::size_t differences(const MapExtrasView& map) const {
        if (!taken_ || map.fields.size() != fields_.size() || map.arrays.size() != arrays_.size())
            return SIZE_MAX;
        std::size_t n = 0;
        for (std::size_t i = 0; i < fields_.size(); ++i)
            if (map.fields[i].size() != fields_[i].size() ||
                std::memcmp(map.fields[i].data(), fields_[i].data(), fields_[i].size()) != 0)
                ++n;
        for (std::size_t i = 0; i < arrays_.size(); ++i) {
            const auto bytes = array_bytes(map.arrays[i]);
            if (bytes.size() != arrays_[i].size() ||
                (!bytes.empty() && std::memcmp(bytes.data(), arrays_[i].data(), bytes.size()) != 0))
                ++n;
        }
        return n;
    }

    // Puts every field and array back. False, with nothing written, when the map does not
    // have the same parts any more or an array cannot take its saved bytes: the caller has
    // to restart.
    template <class Alloc, class Free> bool restore(const MapExtrasView& map, Alloc&& alloc, Free&& free) const {
        if (!taken_ || map.fields.size() != fields_.size() || map.arrays.size() != arrays_.size()) return false;
        for (std::size_t i = 0; i < fields_.size(); ++i)
            if (map.fields[i].size() != fields_[i].size()) return false;
        for (std::size_t i = 0; i < arrays_.size(); ++i)
            if (!map.arrays[i] || !array_takes(map.arrays[i], arrays_[i].size())) return false;
        for (std::size_t i = 0; i < fields_.size(); ++i)
            if (!fields_[i].empty()) std::memcpy(map.fields[i].data(), fields_[i].data(), fields_[i].size());
        for (std::size_t i = 0; i < arrays_.size(); ++i) {
            const auto bytes = array_bytes(map.arrays[i]);
            if (bytes.size() == arrays_[i].size() &&
                (bytes.empty() || std::memcmp(bytes.data(), arrays_[i].data(), bytes.size()) == 0))
                continue;  // unchanged: no write, no block
            put_array(map.arrays[i], arrays_[i], alloc, free);
        }
        return true;
    }
};

}  // namespace hamweard
