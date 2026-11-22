// The center-of-trade modifiers a country gets for its provinces (UpdateCenterOfTradeGlobalModifier:
// operator new of a 0x178-byte CStaticModifier per province, then CCountry::AddTemporaryModifier).
// CCountry::DeleteTemporaryModifier, its "if" variant and ~CCountry delete only the list entry
// (entry+0x30 is the modifier), so every modifier is lost once its entry goes. A country's
// temporary modifiers are a list: head at +0x1F0, nodes of 0x20 bytes (entry, prev, next, flag).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace hamweard {

inline constexpr std::ptrdiff_t country_temporary_modifiers = 0x1F0;
inline constexpr std::ptrdiff_t temporary_node_entry = 0x0;
inline constexpr std::ptrdiff_t temporary_node_next = 0x10;
inline constexpr std::ptrdiff_t temporary_entry_modifier = 0x30;
inline constexpr std::size_t temporary_list_limit = 1 << 20;  // a broken list ends the walk

// The modifiers on one country's temporary list, in list order.
inline std::vector<void*> temporary_modifiers(const std::uint8_t* country) {
    std::vector<void*> modifiers;
    const std::uint8_t* node = nullptr;
    std::memcpy(&node, country + country_temporary_modifiers, sizeof node);
    for (std::size_t n = 0; node && n < temporary_list_limit; ++n) {
        const std::uint8_t* entry = nullptr;
        std::memcpy(&entry, node + temporary_node_entry, sizeof entry);
        if (entry) {
            void* modifier = nullptr;
            std::memcpy(&modifier, entry + temporary_entry_modifier, sizeof modifier);
            if (modifier) modifiers.push_back(modifier);
        }
        std::memcpy(&node, node + temporary_node_next, sizeof node);
    }
    return modifiers;
}

// The modifiers the game made, by the world they were made in: a world starts at each reset and at
// each load of a save. Those of the worlds before the current one are what the game lost.
class ModifierLedger {
public:
    void note(void* modifier) {
        if (modifier) made_[modifier] = world_;
    }
    // The game deleted it after all.
    void forget(void* modifier) { made_.erase(modifier); }
    bool empty() const { return made_.empty(); }
    std::size_t size() const { return made_.size(); }

    // A new world starts; returns it.
    std::uint32_t next_world() { return ++world_; }

    // The modifiers made before `world`, taken out of the ledger.
    std::vector<void*> take_before(std::uint32_t world) {
        std::vector<void*> old;
        for (auto it = made_.begin(); it != made_.end();) {
            if (it->second < world) {
                old.push_back(it->first);
                it = made_.erase(it);
            } else {
                ++it;
            }
        }
        return old;
    }

private:
    std::unordered_map<void*, std::uint32_t> made_;
    std::uint32_t world_ = 0;
};

}  // namespace hamweard
