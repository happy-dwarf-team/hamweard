// Hamweard: Back on the country selection screen and Exit to Menu in a
// campaign return to the main menu instead of restarting the game.
//
// A port of EU4 Menu Patch 0.1.4 (https://github.com/YozoraTempest/EU4-Menu-Patch,
// MIT License, Copyright (c) 2026 VulonLok) to Anvil: the clean-up sequence,
// and the deferred world reset are the original's,
// the hand-written stubs, near allocations and page protection are Anvil's
// replace_block and swap_pointer. Everything runs on the game's main thread.
#include <anvil/anvil.hpp>

#include "map_snapshot.hpp"
#include "menu_transition.hpp"
#include "symbols.gen.hpp"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Bytes = std::uint8_t*;

template <class T> T& at(void* object, std::ptrdiff_t offset) noexcept {
    return *reinterpret_cast<T*>(static_cast<Bytes>(object) + offset);
}

// Structure offsets of 1.37.5, from EU4 Menu Patch; those that differ
// between the builds are in Layout below.
namespace off {
constexpr std::ptrdiff_t app_current_idler = 0x40;
constexpr std::ptrdiff_t app_next_idler = 0x70;
constexpr std::ptrdiff_t app_exit = 0x83;
constexpr std::ptrdiff_t app_restart = 0x84;
constexpr std::ptrdiff_t app_checksum_ready = 0x328;
constexpr std::ptrdiff_t app_services = 0x348;  // its +0x10 is the Steam lobby
constexpr std::ptrdiff_t app_graphical_map = 0x360;
constexpr std::ptrdiff_t app_menu_arg_1 = 0x350;
constexpr std::ptrdiff_t app_menu_arg_2 = 0x358;

constexpr std::ptrdiff_t window_widget = 0x30;          // a window's widget in its full object
constexpr std::ptrdiff_t window_destroy_slot = 0x130;    // of the full object, as the HUD reload (0x82B470) uses it
constexpr std::ptrdiff_t widget_dead = 0xC4;             // as Tongs reads it
constexpr std::ptrdiff_t widget_name_slot = 0xA0;        // returns the .gui name, a std::string*
constexpr std::ptrdiff_t widget_visible = 0xC5;          // as Tongs reads it
constexpr std::ptrdiff_t widget_hide_slot = 0x90;        // of the widget, as 0x82B470 hides the menu bar
constexpr std::ptrdiff_t dialog_name = 0x118;            // eu3dialog: the .gui name of its window (std::string)

constexpr std::ptrdiff_t lobby_id = 0x1C0;
constexpr std::ptrdiff_t lobby_hosting = 0x299;
constexpr std::ptrdiff_t lobby_flag_29b = 0x29B;
constexpr std::ptrdiff_t lobby_searching = 0x29C;
constexpr std::ptrdiff_t lobby_create_call = 0x2B0;
constexpr std::ptrdiff_t lobby_join_call = 0x2E0;
constexpr std::ptrdiff_t lobby_search_callback = 0x300;
constexpr std::ptrdiff_t lobby_search_call = 0x310;

constexpr std::ptrdiff_t world_provinces_begin = 0x1C90;
constexpr std::ptrdiff_t world_provinces_end = 0x1C98;
constexpr std::ptrdiff_t world_graphics_owner = 0x1E00;  // the current idler, as CGameSetup::Back reads it
constexpr std::ptrdiff_t world_potential_corruption = 0x23B4;

constexpr std::ptrdiff_t map_province_graphics = 0x70;
constexpr std::ptrdiff_t province_graphics_size = 0x1400;

constexpr std::ptrdiff_t canal_list_count = 0x14;  // ApplyCanalLogic's argument, a CPdxArray

constexpr std::ptrdiff_t map_templates_begin = 0x6C0;  // CMap: the province templates
constexpr std::ptrdiff_t map_templates_end = 0x6C8;
constexpr std::ptrdiff_t map_naval_count = 0x17C0;
constexpr std::ptrdiff_t map_naval_distance = 0x17C8;
constexpr std::ptrdiff_t template_size = 0x230;
constexpr std::ptrdiff_t template_neighbours_begin = 0x168;  // CProvinceTemplate::_Neighbours
constexpr std::ptrdiff_t template_neighbours_end = 0x170;

constexpr std::ptrdiff_t idler_session_slot = 0x80;  // GetSession
constexpr std::ptrdiff_t session_is_dummy = 0x55;    // CSession::_bIsDummy, single player's session

constexpr std::ptrdiff_t achievements_loaded_begin = 0x88;  // CArray<short>
constexpr std::ptrdiff_t achievements_loaded_end = 0x90;
}  // namespace off

// The offsets that differ on GOG 1.37.5.2 (its eu4.pdb, and the same code in
// both builds): CInGameIdler's fields from its CEU4Application pointer on, and
// CFrontEnd's view state and size. CEU4Application, the widgets and the
// world's PotentialCorruption have the same layout.
struct Layout {
    std::ptrdiff_t idler_app;
    std::ptrdiff_t idler_minimap;
    std::ptrdiff_t idler_exit_flag;
    std::ptrdiff_t idler_notifications;
    std::ptrdiff_t menu_pending;  // CFrontEnd's view state
    std::size_t main_menu_size;
};
constexpr Layout kSteam{0x330, 0x10E0, 0x12E9, 0x1400, 0x900, 0xB48};
constexpr Layout kGog{0x328, 0x10D0, 0x12D9, 0x13F0, 0x8F0, 0xB38};
Layout g_layout = kSteam;

using TickFn = void(void* idler, bool update);
using RestoreFn = void(void* idler);
using UnregisterCallResultFn = void(void* callback, std::uint64_t call);

anvil::PointerSwap<TickFn> g_tick;
anvil::PointerSwap<RestoreFn> g_restore_device_objects;
std::uintptr_t* g_world = nullptr;    // the global holding the root object
std::uintptr_t* g_history = nullptr;  // the global holding the history
void* g_lobby_vtable = nullptr;  // null on a build without the Steam lobby (GOG)
void* g_minimap_vtable = nullptr;
UnregisterCallResultFn* g_unregister_call_result = nullptr;

// Set by the pregame block, taken by the next frontend tick.
std::atomic<void*> g_pending_frontend_app{nullptr};
// Main thread only: the switch to the menu that is under way.
Bytes g_pending_menu_app = nullptr;
void* g_pending_menu_idler = nullptr;
std::uint32_t g_previous_corruption = 0;
std::uint32_t g_returns = 0;

// A canal adds neighbours to map provinces, and the map outlives the world. The
// game's Clear takes the canal's links out again, but then recalculates the costs
// of every link and the naval distances, and those differ from the ones the map
// loaded with (lab: 2233 provinces). So the plugin keeps the map as it was before
// the first canal and puts it back after the reset, or restarts when it cannot.
hamweard::MapSnapshot g_map_before_canals;
bool g_map_changed = false;  // since the last reset
std::uintptr_t* g_map = nullptr;
Bytes g_in_game_stock_path = nullptr;
Bytes g_pregame_stock_path = nullptr;

anvil::HookOf<sym::gui_create_window> g_create_window;
anvil::HookOf<sym::history_execute> g_history_execute;
std::uintptr_t* g_manager_global = nullptr;
std::ptrdiff_t g_registry_begin = 0;
std::ptrdiff_t g_registry_end = 0;
// Main thread: the windows the GUI manager created since the campaign's GUI
// was built, destroyed once the outgoing in-game idler is gone.
bool g_recording_windows = false;
bool g_campaign_seen = false;
// A set, not a list: the game frees most of them itself, and a window made
// later at a freed address (the main menu's country_selection_panel) is not one.
std::unordered_map<void*, std::string> g_campaign_windows;  // full object -> .gui name
// Names of the windows the fresh main menu has: the frontend's.
// The game can create one of them again during a campaign and the main menu
// then uses that one (country_selection_panel), so they are never destroyed.
std::unordered_set<std::string> g_frontend_names;
hamweard::WorldResetGate g_reset_gate;

anvil::Host host() noexcept { return anvil::Host{}; }

template <class S, class... Args> decltype(auto) game(const S& fn, Args&&... args) {
    return host().call(fn, std::forward<Args>(args)...);
}

class BusyScope {
    std::uint8_t previous_ = 0;

public:
    BusyScope() { game(sym::busy_enter, &previous_); }
    ~BusyScope() { game(sym::busy_leave, &previous_); }
    BusyScope(const BusyScope&) = delete;
    BusyScope& operator=(const BusyScope&) = delete;
};

Bytes world() noexcept { return reinterpret_cast<Bytes>(*g_world); }

std::uint32_t potential_corruption() noexcept {
    Bytes w = world();
    return w ? at<std::uint32_t>(w, off::world_potential_corruption) : 0;
}

Bytes steam_lobby(Bytes app) noexcept {
    if (!g_lobby_vtable) return nullptr;
    Bytes services = at<Bytes>(app, off::app_services);
    Bytes lobby = services ? at<Bytes>(services, 0x10) : nullptr;
    return lobby && at<void*>(lobby, 0) == g_lobby_vtable ? lobby : nullptr;
}

void leave_steam_session(Bytes app) {
    Bytes lobby = steam_lobby(app);
    if (!lobby) return;
    auto& search_call = at<std::uint64_t>(lobby, off::lobby_search_call);
    const bool active = at<std::uint64_t>(lobby, off::lobby_id) || at<std::uint64_t>(lobby, off::lobby_create_call) ||
                        at<std::uint64_t>(lobby, off::lobby_join_call) || search_call ||
                        at<std::uint8_t>(lobby, off::lobby_hosting) || at<std::uint8_t>(lobby, off::lobby_flag_29b);
    if (!active) return;
    host().info("Steam lobby clean-up: lobby {:#x}, create {}, join {}, search {}, hosting {}",
                at<std::uint64_t>(lobby, off::lobby_id), at<std::uint64_t>(lobby, off::lobby_create_call) != 0,
                at<std::uint64_t>(lobby, off::lobby_join_call) != 0, search_call != 0,
                at<std::uint8_t>(lobby, off::lobby_hosting));
    // Cancel the lobby search with the same import and callback object as
    // CSteamLobby's destructor, without destroying the reusable lobby.
    if (search_call && g_unregister_call_result) {
        g_unregister_call_result(lobby + off::lobby_search_callback, search_call);
        search_call = 0;
        at<std::uint8_t>(lobby, off::lobby_searching) = 0;
    }
    // Close stops hosting, leaves the lobby and unregisters the lobby event
    // context; Disconnect clears the outstanding join; Initialize registers the
    // context again for the next session.
    game(sym::lobby_close, lobby);
    game(sym::lobby_disconnect, lobby);
    game(sym::lobby_initialize, lobby);
}

// Once the new main menu owns the frame, has finished its first update and the
// checksum is ready. The game's own PotentialCorruption flag stays set: this
// process has played, so the game asks for a restart before multiplayer
// (MUST_RESTART_TO_PLAY_MP). ResetGame only flattens -nudge's reason to 1.
void finish_return(void* idler) {
    if (idler != g_pending_menu_idler || !g_pending_menu_app) return;
    Bytes app = g_pending_menu_app;
    if (at<void*>(app, off::app_current_idler) != idler || at<void*>(app, off::app_next_idler) != nullptr ||
        at<std::uint32_t>(idler, g_layout.menu_pending) != 0 || !at<std::uint8_t>(app, off::app_checksum_ready))
        return;
    Bytes w = world();
    if (!w) return;
    auto& corruption = at<std::uint32_t>(w, off::world_potential_corruption);
    const std::uint32_t before = corruption;
    corruption = hamweard::corruption_after_return(before, g_previous_corruption);
    host().info("main menu active: potential corruption {} -> {} (before the return {}), checksum ready", before,
                corruption, g_previous_corruption);
    g_pending_menu_app = nullptr;
    g_pending_menu_idler = nullptr;
}

// The game's std::string (MSVC x64 layout).
struct GameString {
    char storage[16]{};
    std::size_t size = 0;
    std::size_t capacity = 15;
};
static_assert(sizeof(GameString) == 0x20);

struct ContinueRecord {
    GameString strings[4];
    bool valid = false;
    ContinueRecord() = default;
    ContinueRecord(const ContinueRecord&) = delete;
    ContinueRecord& operator=(const ContinueRecord&) = delete;
    ~ContinueRecord() {
        for (auto& s : strings) game(sym::string_free, &s);
    }
};
static_assert(offsetof(ContinueRecord, valid) == 0x80);

void construct_menu(Bytes app, std::uint32_t reason) {
    // The game reads the Continue game record once at start-up; a return
    // without a restart refreshes the menu's cache after the exit autosave.
    {
        ContinueRecord record;
        game(sym::continue_record_read, &record);
        const GameString empty;
        game(sym::set_save_metadata, app, record.valid ? &record.strings[3] : &empty);
        host().info("Continue game refreshed (record valid: {})", record.valid);
    }
    void* memory = game(sym::game_alloc, std::uint64_t{g_layout.main_menu_size});
    void* menu = game(sym::main_menu_construct, memory, at<void*>(app, off::app_menu_arg_1),
                      at<void*>(app, off::app_menu_arg_2), app);
    // SetNextIdler takes the pointer and nulls the local.
    game(sym::set_next_idler, app, &menu, false);
    g_pending_menu_app = app;
    g_pending_menu_idler = at<void*>(app, off::app_next_idler);
    g_reset_gate.queue(app, g_pending_menu_idler);
    g_previous_corruption = reason;
    ++g_returns;
    host().info("return to the main menu #{} queued (menu {}, exit {}, restart {})", g_returns, g_pending_menu_idler,
                at<std::uint8_t>(app, off::app_exit), at<std::uint8_t>(app, off::app_restart));
}

void release_minimap(Bytes idler) {
    auto& slot = at<void*>(idler, g_layout.idler_minimap);
    void* controller = slot;
    if (!controller) return;
    if (at<void*>(controller, 0) != g_minimap_vtable) {
        host().warn("minimap left alone: unexpected controller vtable");
        return;
    }
    // The in-game idler's exit and destructor do not release this controller.
    // Release it while the outgoing world is still alive.
    slot = nullptr;
    game(sym::minimap_destroy, controller, 1u);
}

std::string_view game_string(const void* string) noexcept;

// Unit banners and the other map objects of the last campaign: ResetGame clears
// them late, and the new menu's first act is to draw the loading screen over the
// GUI root, so they stood on its picture until the menu. This is ResetGame's own
// loop, run before that. The world's graphics owner is the idler that is gone,
// and Clear reads it.
void clear_province_graphics(Bytes app, void* idler) {
    Bytes map = at<Bytes>(app, off::app_graphical_map);
    Bytes w = world();
    if (!map || !w) return;
    at<void*>(w, off::world_graphics_owner) = idler;
    const std::ptrdiff_t provinces =
        (at<Bytes>(w, off::world_provinces_end) - at<Bytes>(w, off::world_provinces_begin)) / 8;
    Bytes graphics = at<Bytes>(map, off::map_province_graphics);
    for (std::ptrdiff_t i = 0; i < provinces; ++i)
        game(sym::province_graphics_clear, graphics + i * off::province_graphics_size);
}

// The new menu's game setup walks the history from the world's date back to the
// start date (CGameSetup::ResetScreen). A world about to be replaced does not
// need that, and after a start from a later date the walk reads relations the
// world has already taken from the history, and crashes.
void on_history_execute(void* history, const void* from, const void* to) noexcept {
    if (g_reset_gate.pending()) {
        host().info("history walk skipped: the world is about to be reset");
        return;
    }
    g_history_execute.next(history, from, to);
}

// The in-game idler and its views create their windows through the GUI
// manager, and nothing destroys them when the idler goes: 1.37.5 restarts the
// process instead. Each campaign left about 90 top-level windows, whose shown
// icons kept lazy sprites (loadType INGAME) counted as shown. An icon loads
// such a texture only when the count was 0, so once the texture was released
// the next campaign drew its freed handle ("Should not be lazy loaded" in
// error.log, then a crash in d3d9; lab, docs/anvil-menu-patch.md).
void note_window(void* window, std::string_view name) {
    if (!g_campaign_seen) {
        g_frontend_names.emplace(name);
    } else if (g_recording_windows && name != "load_screen" && name != "console_wnd" &&
               !g_frontend_names.contains(std::string(name))) {
        g_campaign_windows.insert_or_assign(window, std::string(name));
    } else {
        g_campaign_windows.erase(window);
    }
}

void* on_create_window(void* manager, const void* name) noexcept {
    void* window = g_create_window.next(manager, name);
    if (window) note_window(window, game_string(name));
    return window;
}

void destroy_campaign_windows() {
    std::unordered_map<void*, std::string> windows = std::exchange(g_campaign_windows, {});
    Bytes manager = reinterpret_cast<Bytes>(*g_manager_global);
    if (windows.empty() || !manager) return;
    std::size_t destroyed = 0, gone = 0;
    for (const auto& [window, name] : windows) {
        Bytes widget = static_cast<Bytes>(window) + off::window_widget;
        // Live and not marked dead: the game may have destroyed it already
        // (a closed popup, a HUD reload), or with a window destroyed before.
        void** begin = at<void**>(manager, g_registry_begin);
        void** end = at<void**>(manager, g_registry_end);
        if (std::find(begin, end, widget) == end || at<std::uint8_t>(widget, off::widget_dead) != 0) {
            ++gone;
            continue;
        }
        // The same window, not another one made at its freed address: the
        // widget's name (vtable slot 0xA0, as Tongs reads it) is the one it
        // was created with.
        using NameFn = const void* (*)(void* widget);
        const void* own = at<NameFn>(at<void*>(widget, 0), off::widget_name_slot)(widget);
        if (!own || game_string(own) != name) {
            ++gone;
            continue;
        }
        // Hidden first, as the HUD reload hides the menu bar before it destroys it.
        if (at<std::uint8_t>(widget, off::widget_visible) != 0) {
            using HideFn = void (*)(void* widget);
            at<HideFn>(at<void*>(widget, 0), off::widget_hide_slot)(widget);
        }
        using DestroyFn = void(void* window);
        at<DestroyFn*>(at<void*>(window, 0), off::window_destroy_slot)(window);
        ++destroyed;
    }
    host().info("windows of the last campaign: {} destroyed, {} already gone", destroyed, gone);
}

std::string_view game_string(const void* string) noexcept {
    const auto* s = static_cast<const GameString*>(string);
    const char* data = s->capacity > 15 ? *reinterpret_cast<char* const*>(s->storage) : s->storage;
    return {data, s->size};
}

hamweard::MapView map_view() {
    hamweard::MapView view;
    auto* map = reinterpret_cast<Bytes>(*g_map);
    if (!map) return view;
    Bytes begin = at<Bytes>(map, off::map_templates_begin);
    Bytes end = at<Bytes>(map, off::map_templates_end);
    view.neighbours.reserve(static_cast<std::size_t>((end - begin) / off::template_size));
    for (Bytes t = begin; t != end; t += off::template_size) {
        auto* first = at<std::uint8_t*>(t, off::template_neighbours_begin);
        auto* last = at<std::uint8_t*>(t, off::template_neighbours_end);
        view.neighbours.emplace_back(first, last);
    }
    const auto count = static_cast<std::size_t>(at<std::int32_t>(map, off::map_naval_count));
    view.naval = {at<std::uint8_t*>(map, off::map_naval_distance), count * count * 4};
    return view;
}

// CAchievementsManager is a singleton, so ResetGame leaves the list the last
// loaded save carried in it, and a restart cleared that: every later save whose
// achievements are off would write the list as its achievement={...} block.
void forget_loaded_achievements() {
    auto* manager = static_cast<Bytes>(game(sym::achievements_instance));
    at<void*>(manager, off::achievements_loaded_end) = at<void*>(manager, off::achievements_loaded_begin);
}

// Whether the idler's session is a real one, not single player's dummy. Anvil
// suspends the plugin only for IsMultiplayerWithPlayers during a campaign, which
// leaves out a lobby and a host alone in the game, and only the game's restart
// ends those sessions.
bool in_multiplayer_session() {
    Bytes w = world();
    void* idler = w ? at<void*>(w, off::world_graphics_owner) : nullptr;
    if (!idler) return false;
    using GetSessionFn = Bytes (*)(void* idler);
    Bytes session = at<GetSessionFn>(at<void*>(idler, 0), off::idler_session_slot)(idler);
    return session && !at<std::uint8_t>(session, off::session_is_dummy);
}

// In CInGameIdler::Idle, instead of setting exit + restart.
void return_to_menu(Bytes idler) {
    Bytes app = at<Bytes>(idler, g_layout.idler_app);
    const std::uint32_t reason = potential_corruption();
    host().info("Exit to Menu: idler {}, world {}, potential corruption {}", static_cast<void*>(idler),
                static_cast<void*>(world()), reason);
    g_recording_windows = false;
    at<std::uint8_t>(idler, g_layout.idler_exit_flag) = 0;
    BusyScope busy;
    game(sym::in_game_exit, idler);
    leave_steam_session(app);
    game(sym::campaign_exit_cleanup);
    // The in-game reload clears this owner before it replaces the world; only
    // its full destructor removes the GUI and the callbacks too.
    if (void* notices = at<void*>(idler, g_layout.idler_notifications)) game(sym::notifications_destroy, notices);
    release_minimap(idler);
    construct_menu(app, reason);
}

// The frontend idler's first act after the hand-over: the outgoing idler is gone.
void on_frontend_restore(void* idler) {
    if (g_pending_menu_app && idler == g_pending_menu_idler) clear_province_graphics(g_pending_menu_app, idler);
    g_restore_device_objects.previous()(idler);
}

// The frontend idler's tick: the queued Back, then the deferred world reset.
void on_frontend_tick(void* idler, bool update) {
    if (auto* app = static_cast<Bytes>(g_pending_frontend_app.exchange(nullptr))) {
        host().info("Back: returning from the country selection");
        const std::uint32_t reason = potential_corruption();
        BusyScope busy;
        leave_steam_session(app);
        construct_menu(app, reason);
        return;
    }
    if (Bytes app = g_pending_menu_app) {
        g_reset_gate.run(app, idler, at<void*>(app, off::app_current_idler), at<void*>(app, off::app_next_idler),
                         [&] {
                             using Clock = std::chrono::steady_clock;
                             const auto ms = [](Clock::time_point from, Clock::time_point to) {
                                 return std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count();
                             };
                             BusyScope busy;
                             const auto start = Clock::now();
                             destroy_campaign_windows();
                             const auto windows = Clock::now();
                             game(sym::reset_game, app, false);
                             forget_loaded_achievements();
                             if (g_map_changed) {
                                 g_map_changed = false;
                                 if (g_map_before_canals.restore(map_view())) {
                                     host().info("map put back as it was before the canals");
                                 } else {
                                     // A campaign must not start on a map with a canal's costs.
                                     host().warn("the map could not be put back after a canal: the game restarts");
                                     at<std::uint16_t>(app, off::app_exit) = 0x0101;
                                 }
                             }
                             const auto reset = Clock::now();
                             // A frontend owns this reset, so the history needs the
                             // post-load validation ResetGame runs only for an in-game idler.
                             game(sym::history_post_validate, reinterpret_cast<void*>(*g_history));
                             host().info("world reset (world {}): windows {} ms, reset {} ms, history {} ms",
                                         static_cast<void*>(world()), ms(start, windows), ms(windows, reset),
                                         ms(reset, Clock::now()));
                         });
    }
    g_tick.previous()(idler, update);
    finish_return(idler);
}

struct Hamweard : anvil::Plugin {
    anvil::Change in_game;
    anvil::Change pregame;
    anvil::Change gui_init;
    anvil::Change idler_construct;
    anvil::Change dialog_store;
    anvil::Change canal_logic;

    void load(anvil::Host& h) override {
        g_world = h.data(sym::world);
        g_history = h.data(sym::history);
        g_map = h.data(sym::map);
        const char* build = h.game().build_id;
        g_layout = build && std::string_view(build) == "eu4-1.37.5.2-gog-win64" ? kGog : kSteam;
        g_minimap_vtable = h.address(sym::minimap_vtable);
        // The GOG build has no Steam lobby: its four symbols are absent there.
        if (h.present(sym::steam_lobby_vtable)) {
            g_lobby_vtable = h.address(sym::steam_lobby_vtable);
            // The import CSteamLobby's destructor calls through (IAT 0x1B67730).
            if (HMODULE steam = GetModuleHandleW(L"steam_api64.dll"))
                g_unregister_call_result = reinterpret_cast<UnregisterCallResultFn*>(
                    reinterpret_cast<void*>(GetProcAddress(steam, "SteamAPI_UnregisterCallResult")));
            if (!g_unregister_call_result) h.warn("SteamAPI_UnregisterCallResult not found: a lobby search is not cancelled");
        }
        h.info("game build {}: {}", build ? build : "unknown", g_lobby_vtable ? "with the Steam lobby clean-up" : "no Steam lobby");

        // The three changes are enabled together after load(), or none.
        g_in_game_stock_path = static_cast<Bytes>(h.address(sym::in_game_menu_block)) + 7;
        g_pregame_stock_path = static_cast<Bytes>(h.address(sym::pregame_menu_block)) + 9;
        in_game = h.replace_block(sym::in_game_menu_block, [](anvil::Cpu& cpu) {
            if (in_multiplayer_session()) {
                host().info("Exit to Menu in a multiplayer session: the game restarts");
                cpu.rcx = cpu.rbp + 0x240;  // the block's own lea
                cpu.rip = reinterpret_cast<std::uintptr_t>(g_in_game_stock_path);
                return;
            }
            return_to_menu(reinterpret_cast<Bytes>(cpu.r13));
        });
        pregame = h.replace_block(sym::pregame_menu_block, [](anvil::Cpu& cpu) {
            if (in_multiplayer_session()) {
                host().info("Back in a multiplayer lobby: the game restarts");
                at<std::uint16_t>(reinterpret_cast<void*>(cpu.rbx), off::app_exit) = 0x0101;  // the block's own mov
                cpu.rip = reinterpret_cast<std::uintptr_t>(g_pregame_stock_path);
                return;
            }
            g_pending_frontend_app.store(reinterpret_cast<void*>(cpu.rbx));
        });
        canal_logic = h.mid_hook(sym::canal_logic_apply, [](anvil::Cpu& cpu) {
            if (at<std::int32_t>(reinterpret_cast<void*>(cpu.rcx), off::canal_list_count) > 0) {
                if (!g_map_before_canals.taken()) {
                    g_map_before_canals.take(map_view());
                    host().info("map kept before the first canal");
                }
                g_map_changed = true;
            }
        });
        h.swap(g_tick, sym::frontend_tick_slot, &on_frontend_tick);
        h.swap(g_restore_device_objects, sym::frontend_restore_slot, &on_frontend_restore);
        g_manager_global = reinterpret_cast<std::uintptr_t*>(h.game().image_base + h.value(sym::gui_manager_store, "manager"));
        g_registry_begin = static_cast<std::ptrdiff_t>(h.value(sym::widget_registry, "begin"));
        g_registry_end = static_cast<std::ptrdiff_t>(h.value(sym::widget_registry, "end"));
        h.hook(g_create_window, sym::gui_create_window, on_create_window);
        h.hook(g_history_execute, sym::history_execute, on_history_execute);
        // Recording starts with the in-game idler (its views make windows
        // while a save loads) and again at each GUI build (the HUD reload).
        const auto record = [] {
            g_campaign_seen = true;
            g_recording_windows = true;
        };
        idler_construct = h.mid_hook(sym::in_game_idler_construct, record);
        gui_init = h.mid_hook(sym::in_game_gui_init, record);
        // Message popups (eu3dialog) build their window themselves, not
        // through the manager's create; each kept its settings icon counted
        // as shown (MessageSettingsButton's count equalled the leaked popups).
        dialog_store = h.mid_hook(sym::dialog_window_store, [](anvil::Cpu& cpu) {
            if (void* window = reinterpret_cast<void*>(cpu.rdi))
                note_window(window, game_string(reinterpret_cast<Bytes>(cpu.rbx) + off::dialog_name));
        });
    }
};

}  // namespace

ANVIL_PLUGIN(Hamweard)
