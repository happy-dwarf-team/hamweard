#pragma once

#include <string_view>

namespace hamweard {

// The structure layout a build of symbols.toml uses. Anvil loads the plugin only
// on a build the file declares; a declared build Hamweard has no layout for
// refuses the load instead of reading the Steam offsets in another build.
enum class BuildLayout { steam, gog, unknown };

constexpr BuildLayout build_layout(std::string_view build) noexcept {
    if (build.ends_with("-gog-win64")) return BuildLayout::gog;
    if (build.ends_with("-steam-win64")) return BuildLayout::steam;
    return BuildLayout::unknown;
}

constexpr const char* layout_name(BuildLayout layout) noexcept {
    switch (layout) {
        case BuildLayout::steam: return "Steam";
        case BuildLayout::gog: return "GOG";
        default: return "unknown";
    }
}

}  // namespace hamweard
