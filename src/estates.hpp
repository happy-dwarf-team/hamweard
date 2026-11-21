// A country's estates (CCountry::_Estates, a CArray<CEstate> of 0xA8-byte elements, begin and
// end at +0 and +8). Each estate keeps its temporary influence and loyalty modifiers as
// CArray<CEstateValue*> (+0x58 and +0x70): it deletes one when it expires
// (CEstate::UpdateInfluenceAndLoyaltyLevels), but ~CEstate frees only the arrays' buffers.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace hamweard {

inline constexpr std::ptrdiff_t country_estates = 0x21B0;
inline constexpr std::size_t estate_size = 0xA8;
inline constexpr std::ptrdiff_t estate_value_arrays[2] = {0x58, 0x70};

// The values of every estate of one country, and each array emptied (end = begin), so that
// ~CEstate then frees only the buffers. The caller deletes the values.
inline std::vector<std::uint8_t*> take_estate_values(std::uint8_t* country) {
    std::vector<std::uint8_t*> values;
    std::uint8_t* begin = nullptr;
    std::uint8_t* end = nullptr;
    std::memcpy(&begin, country + country_estates, sizeof begin);
    std::memcpy(&end, country + country_estates + 8, sizeof end);
    if (!begin || end < begin) return values;
    for (std::uint8_t* estate = begin; estate + estate_size <= end; estate += estate_size) {
        for (std::ptrdiff_t array : estate_value_arrays) {
            std::uint8_t** first = nullptr;
            std::uint8_t** last = nullptr;
            std::memcpy(&first, estate + array, sizeof first);
            std::memcpy(&last, estate + array + 8, sizeof last);
            if (!first || last <= first) continue;
            for (std::uint8_t** v = first; v < last; ++v)
                if (*v) values.push_back(*v);
            std::memcpy(estate + array + 8, &first, sizeof first);
        }
    }
    return values;
}

}  // namespace hamweard
