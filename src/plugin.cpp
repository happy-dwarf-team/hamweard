// Hamweard: Back on the country selection screen and Exit to Menu in a
// campaign return to the main menu instead of restarting the game.
//
// A port of EU4 Menu Patch 0.1.4 (https://github.com/YozoraTempest/EU4-Menu-Patch,
// MIT License, Copyright (c) 2026 VulonLok) to Anvil: the clean-up sequence,
// and the deferred world reset are the original's,
// the hand-written stubs, near allocations and page protection are Anvil's
// replace_block and swap_pointer. Everything runs on the game's main thread.
#include <anvil/anvil.hpp>

#include "build_layout.hpp"
#include "estates.hpp"
#include "game_switches.hpp"
#include "leader_table.hpp"
#include "map_snapshot.hpp"
#include "menu_transition.hpp"
#include "strength_caches.hpp"
#include "symbols.gen.hpp"

#include <windows.h>
// After windows.h: PSAPI_VERSION 2 maps GetProcessMemoryInfo to kernel32's K32 export.
#include <psapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <mutex>
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
constexpr std::ptrdiff_t goto_box_window = 0x30;         // CGotoBox::_pWindow, "goto_box"
constexpr std::ptrdiff_t tutorial_minimized = 0xE0;      // CTutorial::_pMinimized, "tutorial_minimized"
constexpr std::ptrdiff_t game_setup_view = 0x28;         // CGameSetup's CFrontEndView base, as CFrontEnd::_Views holds it
constexpr std::ptrdiff_t game_setup_start_dialog = 0x2CA8;  // CGameSetup::_pStartDialog
constexpr std::ptrdiff_t start_dialog_ironman = 0x298;   // CStartGameDialog::_pIronmanSaveSelect

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
constexpr std::ptrdiff_t world_province_objects = 0x1CA8;  // CProvince[], as ResetGame and CGameSetup walk them
constexpr std::ptrdiff_t province_size = 0x2E10;
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
constexpr std::ptrdiff_t idler_validates_history_slot = 0x140;  // asked by ResetGame before PostValidateEffects
constexpr std::ptrdiff_t session_is_dummy = 0x55;    // CSession::_bIsDummy, single player's session

constexpr std::ptrdiff_t achievements_loaded_begin = 0x88;  // CArray<short>
constexpr std::ptrdiff_t achievements_loaded_end = 0x90;

constexpr std::ptrdiff_t if_effect_else = 0xF0;          // CIfEffect::_pElse, the next else_if/else branch
constexpr std::ptrdiff_t country_db_count = 0x100;       // CCountryDataBase::_nNumberOfCountries
constexpr std::ptrdiff_t country_db_countries = 0x118;   // its CCountry* array, as ReadCountryFiles walks it
constexpr std::ptrdiff_t country_has_idea_group = 0x10A0;  // CCountry::_HasIdeaGroup, a bool[]
constexpr std::ptrdiff_t subject_modifiers_begin = 0x6440;  // CCountryModifiers::ModifiersFromSubjectToOverlord
constexpr std::ptrdiff_t subject_modifiers_end = 0x6448;
constexpr std::ptrdiff_t country_delayed_begin = 0x2200;  // CCountry::_DelayedEvents, a vector of CDelayedEvent*
constexpr std::ptrdiff_t country_delayed_end = 0x2208;
constexpr std::ptrdiff_t fired_events = 0x168;  // DailyUpdateSerial's rbp: the events it fired today (their data)
constexpr std::ptrdiff_t fired_count = 0x174;   // and their count

constexpr std::ptrdiff_t monarch_religion = 0x58;   // CMonarch::_pReligion
constexpr std::ptrdiff_t monarch_culture = 0x60;    // CMonarch::_pCulture
constexpr std::ptrdiff_t monarch_dynasty = 0x68;    // CMonarch::_pDynasty
constexpr std::ptrdiff_t monarch_flags = 0xA0;      // CMonarch::_RulerFlags, a CIntegerFlags
constexpr std::ptrdiff_t flags_begin = 0x10;        // CIntegerFlags::_Flags, a CArray<short>
constexpr std::ptrdiff_t flags_end = 0x18;
constexpr std::ptrdiff_t culture_name = 0x48;       // CCulture's tag (std::string)

constexpr std::ptrdiff_t gui_gestures_begin = 0x90;   // CGuiObject: the recognizers it owns, {pointer, owned} of 16 bytes
constexpr std::ptrdiff_t gui_gestures_end = 0x98;
constexpr std::ptrdiff_t gui_gesture_size = 0x10;
constexpr std::ptrdiff_t text_box_observer = 0xC8;    // CInstantTextBox's CGestureObserver base
constexpr std::ptrdiff_t text_box_pan = 0x210;        // CInstantTextBox::_pPanGesture
constexpr std::ptrdiff_t gesture_fingers_begin = 0x8;    // CGestureRecognizer::_TrackedFingers
constexpr std::ptrdiff_t gesture_fingers_end = 0x10;
constexpr std::ptrdiff_t gesture_observers_begin = 0x28;  // CGestureRecognizer::_Observers, CGestureObserver*
constexpr std::ptrdiff_t gesture_observers_end = 0x30;
constexpr std::ptrdiff_t gesture_waiters_begin = 0x40;    // CGestureRecognizer::_FailureWaiters
constexpr std::ptrdiff_t gesture_waiters_end = 0x48;
}  // namespace off

// The offsets that differ on the GOG builds 1.37.5.2 and 1.37.5.0 (their
// eu4.pdb, and the same code in all builds): CInGameIdler's fields from its
// CEU4Application pointer on, and CFrontEnd's view state and size.
// CEU4Application, the widgets and the world's PotentialCorruption have the
// same layout. Both GOG builds have the same layouts.
struct Layout {
    std::ptrdiff_t idler_app;
    std::ptrdiff_t idler_minimap;
    std::ptrdiff_t idler_exit_flag;
    std::ptrdiff_t idler_notifications;
    std::ptrdiff_t menu_pending;  // CFrontEnd's view state
    std::size_t main_menu_size;
    std::ptrdiff_t frontend_right_click;  // CFrontEnd::_pRightClickWindow
    std::ptrdiff_t idler_goto_box;        // CInGameIdler::_pGotoBox
    std::ptrdiff_t idler_tutorial;        // CInGameIdler::_pTutorial
    std::ptrdiff_t idler_right_click;     // CInGameIdler::_pRightClickWindow
};
constexpr Layout kSteam{0x330, 0x10E0, 0x12E9, 0x1400, 0x900, 0xB48, 0x7C8, 0x1428, 0x1440, 0x1448};
constexpr Layout kGog{0x328, 0x10D0, 0x12D9, 0x13F0, 0x8F0, 0xB38, 0x7B8, 0x1418, 0x1430, 0x1438};
Layout g_layout = kSteam;


using TickFn = void(void* idler, bool update);
using RestoreFn = void(void* idler);
using UnregisterCallResultFn = void(void* callback, std::uint64_t call);

anvil::PointerSwap<TickFn> g_tick;
anvil::PointerSwap<RestoreFn> g_restore_device_objects;
std::uintptr_t* g_world = nullptr;    // the global holding the root object
std::uintptr_t* g_history = nullptr;  // the global holding the history
std::uintptr_t* g_trigger_db = nullptr;  // CTriggerDataBase's instance: alive only while the game loads
std::uint8_t* g_custom_nation_used = nullptr;  // only a restart clears it, and the lobby reads it
std::uintptr_t* g_country_db = nullptr;
const std::uintptr_t* g_strength_cache_list = nullptr;   // CacheList (strength_caches.hpp)
const std::uintptr_t* g_strength_cache_mutex = nullptr;  // CacheListMutex
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
// Random New World rewrites the map. The game's own restore misses part of it (lab: the
// next campaign gave Tortuga other missions than a fresh start), and only a restart
// reads the map again, so this process has no clean return left.
bool g_map_randomized = false;
std::uintptr_t* g_map = nullptr;
Bytes g_in_game_stock_path = nullptr;
Bytes g_pregame_stock_path = nullptr;

anvil::HookOf<sym::gui_create_window> g_create_window;
anvil::HookOf<sym::history_execute> g_history_execute;
anvil::HookOf<sym::reset_game> g_reset_game;
anvil::HookOf<sym::multiplayer_click> g_multiplayer_click;
anvil::HookOf<sym::multiplayer_availability> g_multiplayer_availability;
anvil::HookOf<sym::if_effect_delete> g_if_effect_delete;
anvil::HookOf<sym::history_move> g_history_move;
anvil::HookOf<sym::country_modifiers_destroy> g_country_modifiers_destroy;
anvil::HookOf<sym::history_destroy> g_history_destroy;
anvil::HookOf<sym::save_load_task> g_save_load_task;
anvil::HookOf<sym::text_box_destroy> g_text_box_destroy;
bool g_returned_to_menu = false;
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
// Windows whose owner was destroyed without destroying them, and the start game
// dialogs of destroyed game setup views; freed at the next return (see
// on_goto_box_delete). Guarded: views also create windows while a save loads.
std::mutex g_orphans_mutex;
std::unordered_map<void*, std::string_view> g_orphan_windows;  // full object -> .gui name
std::unordered_set<void*> g_orphan_dialogs;
void* g_start_dialog_vtable = nullptr;
void* g_ironman_select_vtable = nullptr;
void* g_goto_box_vtable = nullptr;
void* g_tutorial_vtable = nullptr;
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
    g_returned_to_menu = true;
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

// ResetGame reloads the history files, and their triggers bind names like a
// government reform only in the validation InitGame runs once, through a database
// that exists only there. So a history effect's has_reform stayed false after a
// return, which changed a new Anbennar campaign (lab: S9). The database is created
// for the reset and used before its first walk, where InitGame uses it, then the
// effects get the validation ResetGame runs only for an in-game idler.
bool g_history_reloading = false;

void validate_reloaded_history() {
    if (!g_history_reloading) return;
    g_history_reloading = false;
    if (void* triggers = reinterpret_cast<void*>(*g_trigger_db)) {
        game(sym::trigger_db_validate, triggers);
        game(sym::trigger_db_destroy);
    }
    game(sym::history_post_validate, reinterpret_cast<void*>(*g_history));
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
    validate_reloaded_history();
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
    if (window) {
        note_window(window, game_string(name));
        std::lock_guard lock(g_orphans_mutex);
        g_orphan_windows.erase(window);
    }
    return window;
}

// Destroys a window that is still live and still the one named `name`; false
// when the game destroyed it already.
bool destroy_window(Bytes manager, void* window, std::string_view name) {
    Bytes widget = static_cast<Bytes>(window) + off::window_widget;
    // Live and not marked dead: the game may have destroyed it already
    // (a closed popup, a HUD reload), or with a window destroyed before.
    void** begin = at<void**>(manager, g_registry_begin);
    void** end = at<void**>(manager, g_registry_end);
    if (std::find(begin, end, widget) == end || at<std::uint8_t>(widget, off::widget_dead) != 0) return false;
    // The same window, not another one made at its freed address: the
    // widget's name (vtable slot 0xA0, as Tongs reads it) is the one it
    // was created with.
    using NameFn = const void* (*)(void* widget);
    const void* own = at<NameFn>(at<void*>(widget, 0), off::widget_name_slot)(widget);
    if (!own || game_string(own) != name) return false;
    // Hidden first, as the HUD reload hides the menu bar before it destroys it.
    if (at<std::uint8_t>(widget, off::widget_visible) != 0) {
        using HideFn = void (*)(void* widget);
        at<HideFn>(at<void*>(widget, 0), off::widget_hide_slot)(widget);
    }
    using DestroyFn = void(void* window);
    at<DestroyFn*>(at<void*>(window, 0), off::window_destroy_slot)(window);
    return true;
}

// The windows their owners left and the start game dialogs of the destroyed
// game setup views. A dialog is freed by its own deleting destructor, which
// destroys its window and leaves the GUI reload list; its ironman save
// selection, which it never frees, first, the same way.
void free_orphans(Bytes manager) {
    std::unique_lock lock(g_orphans_mutex);
    const auto windows = std::exchange(g_orphan_windows, {});
    const auto dialogs = std::exchange(g_orphan_dialogs, {});
    lock.unlock();
    if (windows.empty() && dialogs.empty()) return;
    using DeleteFn = void*(void* object, std::uint32_t flags);
    std::size_t destroyed = 0, freed = 0;
    for (const auto& [window, name] : windows) destroyed += destroy_window(manager, window, name) ? 1 : 0;
    for (void* dialog : dialogs) {
        if (at<void*>(dialog, 0) != g_start_dialog_vtable) continue;
        auto& select = at<void*>(dialog, off::start_dialog_ironman);
        if (void* ironman = select; ironman && at<void*>(ironman, 0) == g_ironman_select_vtable) {
            select = nullptr;
            at<DeleteFn*>(g_ironman_select_vtable, 0)(ironman, 1);
        }
        at<DeleteFn*>(g_start_dialog_vtable, 0)(dialog, 1);
        ++freed;
    }
    host().info("windows their owners left: {} of {} destroyed; {} start game dialogs freed", destroyed, windows.size(),
                freed);
}

void destroy_campaign_windows() {
    std::unordered_map<void*, std::string> windows = std::exchange(g_campaign_windows, {});
    Bytes manager = reinterpret_cast<Bytes>(*g_manager_global);
    if (!manager) return;
    free_orphans(manager);
    if (windows.empty()) return;
    std::size_t destroyed = 0, gone = 0;
    for (const auto& [window, name] : windows) {
        if (destroy_window(manager, window, name)) {
            ++destroyed;
        } else {
            ++gone;
        }
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

// The game's own resets on the frontend. A save picked in the Single Player list
// and then a start date or a bookmark make the setup screen run ResetGame, which
// reloads the history files but validates their effects (PostValidateEffects,
// after the trigger database) only for an idler that asks for it, and the
// frontend's does not. So the history's add_province_triggered_modifier and the
// like stayed unbound, and the campaign started without any province triggered
// modifier, as without Hamweard (lab: pol-k1; hwr-fix2 traced the reset). They get
// the validation of Hamweard's return, which a fresh start matches. Not in a
// multiplayer lobby, where every machine must run the reset the same way.
// ResetGame gives every province its plain name back (ResetName); the dynamic
// names after the owner's culture come from CGameSetup's constructor, which runs
// UpdateDynamicProvinceName on every province when the setup screen opens. After a
// save is clicked in the list, the setup screen is already open, so a campaign
// started from a bookmark or date kept the plain names (Jämtland for Jemtland,
// 259 of 4941 provinces in the lab, hwr-names; stock game too). The frontend
// reset gets them back, as the setup screen of a fresh start gives them.
void name_provinces_as_setup() {
    Bytes w = world();
    if (!w) return;
    const std::ptrdiff_t count =
        (at<Bytes>(w, off::world_provinces_end) - at<Bytes>(w, off::world_provinces_begin)) / 8;
    Bytes provinces = at<Bytes>(w, off::world_province_objects);
    if (!provinces) return;
    for (std::ptrdiff_t i = 0; i < count; ++i) game(sym::province_update_dynamic_name, provinces + i * off::province_size);
    host().info("dynamic province names set again after the frontend reset ({} provinces)", count);
}

void on_reset_game(void* app, bool keep_history) noexcept {
    const auto idler_validates = [app] {
        void* idler = at<void*>(app, off::app_current_idler);
        using ValidatesFn = bool (*)(void* idler);
        return !idler || at<ValidatesFn>(at<void*>(idler, 0), off::idler_validates_history_slot)(idler);
    };
    const bool frontend_reset =
        hamweard::reset_needs_validation(keep_history, g_history_reloading, in_multiplayer_session, idler_validates);
    if (frontend_reset) {
        host().info("the game resets the world on the frontend: its history gets the validation of a fresh start");
        if (!*g_trigger_db) game(sym::trigger_db_create);
        g_history_reloading = true;  // validated at the reset's first walk of the history
    }
    g_reset_game.next(app, keep_history);
    validate_reloaded_history();  // a reset that did not walk the history
    if (frontend_reset) name_provinces_as_setup();
}

void on_multiplayer_click(void* state) {
    if (hamweard::multiplayer_needs_restart(g_returned_to_menu, potential_corruption())) {
        host().info("Multiplayer after a return: the game has to restart first");
        return;
    }
    g_multiplayer_click.next(state);
}

int on_multiplayer_availability(void* view) {
    return hamweard::multiplayer_availability(g_multiplayer_availability.next(view), g_returned_to_menu,
                                              potential_corruption());
}

// Whether Shift is down now. Both blocks run on the main thread in the frame
// that handles the click that leaves (Quit on the end-of-game screen, Back), a
// moment after it, so the key's physical state is the one of that click;
// GetKeyState adds what the window's messages last said.
bool shift_held() {
    return ((GetAsyncKeyState(VK_SHIFT) | GetKeyState(VK_SHIFT)) & 0x8000) != 0;
}

// The process's private memory and the system's commit left, for the log of a
// return: what a report needs to show a return's growth or a low-memory restart.
std::string memory_figures() {
    constexpr std::uint64_t mb = 1024 * 1024;
    PROCESS_MEMORY_COUNTERS_EX process{};
    process.cb = sizeof process;
    MEMORYSTATUSEX system{};
    system.dwLength = sizeof system;
    const bool own = GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&process),
                                          sizeof process);
    const bool left = GlobalMemoryStatusEx(&system);
    return std::format("private {}, commit left {}",
                       own ? std::format("{} MB", process.PrivateUsage / mb) : std::string("unknown"),
                       left ? std::format("{} MB", system.ullAvailPageFile / mb) : std::string("unknown"));
}

// Why a return has to be the game's own restart, or null when it does not.
const char* restart_reason() {
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof memory;
    const std::uint64_t commit = GlobalMemoryStatusEx(&memory) ? memory.ullAvailPageFile : UINT64_MAX;
    return hamweard::restart_reason(shift_held(), in_multiplayer_session(), g_map_randomized, commit);
}

void note_idler_orphans(Bytes idler);

// In CInGameIdler::Idle, instead of setting exit + restart.
void return_to_menu(Bytes idler) {
    Bytes app = at<Bytes>(idler, g_layout.idler_app);
    const std::uint32_t reason = potential_corruption();
    host().info("Exit to Menu: idler {}, world {}, potential corruption {}", static_cast<void*>(idler),
                static_cast<void*>(world()), reason);
    g_recording_windows = false;
    at<std::uint8_t>(idler, g_layout.idler_exit_flag) = 0;
    BusyScope busy;
    game(sym::in_game_exit, idler);  // NiceMultiplayerExit: a new single-player session
    leave_steam_session(app);
    game(sym::campaign_exit_cleanup);  // HandleIronmanSave: an ironman campaign is saved here
    // The alert manager's diplomatic icons (ClearAllDiploIcons): a save's load and a
    // tag switch destroy their windows before the world changes, and so does a return.
    if (void* notices = at<void*>(idler, g_layout.idler_notifications)) game(sym::notifications_destroy, notices);
    release_minimap(idler);
    note_idler_orphans(idler);
    construct_menu(app, reason);
}

// ResetGame destroys the history and reads it again. CIfEffect's destructor frees
// the if but not _pElse, the else_if/else branch AddElse chained to it, which only
// that pointer holds; so each reset lost every such branch with its subtree (lab:
// about 60 MB per return with Anbennar, whose country history calls scripted effects
// full of else_if). The game loses them the same way where it deletes the history's
// effects elsewhere: the start of a new campaign (MoveIntoGameState: effects cannot
// move into the game state) and the load of a save (DestroyDatabase). Only in those
// windows: in the rest of the game, the game's exit among it, the detour passes the
// call on unchanged. A counter, as the history's destruction runs inside the reset.
std::atomic<int> g_free_else{0};
std::atomic<int> g_loading_save{0};  // inside CGameStateLoadTask, which destroys the history first

class FreeElseWindow {
public:
    FreeElseWindow() { g_free_else.fetch_add(1, std::memory_order_relaxed); }
    ~FreeElseWindow() { g_free_else.fetch_sub(1, std::memory_order_relaxed); }
    FreeElseWindow(const FreeElseWindow&) = delete;
    FreeElseWindow& operator=(const FreeElseWindow&) = delete;
};
std::atomic<std::uint32_t> g_else_branches_freed{0};

void* on_if_effect_delete(void* effect, std::uint32_t flags) noexcept {
    if (g_free_else.load(std::memory_order_relaxed) <= 0) return g_if_effect_delete.next(effect, flags);
    void* branch = std::exchange(at<void*>(effect, off::if_effect_else), nullptr);
    void* result = g_if_effect_delete.next(effect, flags);
    if (branch) {
        // Slot 0 with 1, as the game deletes an effect: a CIfEffect again, so a
        // chain of branches goes through this detour one by one.
        using DeleteFn = void* (*)(void* effect, std::uint32_t flags);
        at<DeleteFn>(at<void*>(branch, 0), 0)(branch, 1);
        g_else_branches_freed.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

// ReadCountryFiles gives each country a bool[] of its idea groups (operator new[]),
// and CCountry's destructor never frees it, so each reset lost one per country
// (lab: 2.6 MB with Anbennar, 0.8 MB without). Freed with the game's own delete
// just before the reset destroys the countries; the countries that the reset reads
// get a new one, and the game checks for a missing one before it clears it.
// A country's estates own their temporary influence and loyalty modifiers (CEstateValue,
// 0xB8 bytes with a trigger and a name): an estate deletes one when it expires, but ~CEstate
// frees only the arrays that hold them, so each reset lost every one still running (lab, 19
// November 2026: about 0.2 MB per return after a 1719 save). Hamweard deletes them, through
// their own destructor, just before the reset destroys the countries, and empties the arrays.
// A value is deleted only if it is a CEstateValue by its exact vtable.
void* g_estate_value_vtable = nullptr;

std::size_t free_estate_values() {
    Bytes db = reinterpret_cast<Bytes>(*g_country_db);
    if (!db || !g_estate_value_vtable) return 0;
    const std::int32_t count = at<std::int32_t>(db, off::country_db_count);
    Bytes* countries = at<Bytes*>(db, off::country_db_countries);
    if (!countries) return 0;
    std::size_t freed = 0;
    for (std::int32_t i = 0; i < count; ++i) {
        if (!countries[i]) continue;
        for (Bytes value : hamweard::take_estate_values(countries[i])) {
            if (at<void*>(value, 0) != g_estate_value_vtable) continue;  // left to the heap, as before
            using ValueDeleteFn = void* (*)(void* value, std::uint32_t flags);
            at<ValueDeleteFn>(g_estate_value_vtable, 0)(value, 1);
            ++freed;
        }
    }
    return freed;
}

std::size_t free_idea_group_flags() {
    Bytes db = reinterpret_cast<Bytes>(*g_country_db);
    if (!db) return 0;
    const std::int32_t count = at<std::int32_t>(db, off::country_db_count);
    Bytes* countries = at<Bytes*>(db, off::country_db_countries);
    if (!countries) return 0;
    std::size_t freed = 0;
    for (std::int32_t i = 0; i < count; ++i) {
        if (!countries[i]) continue;
        if (void* flags = std::exchange(at<void*>(countries[i], off::country_has_idea_group), nullptr)) {
            game(sym::game_free, flags);
            ++freed;
        }
    }
    return freed;
}

// A country's modifiers are rebuilt in place (CalculateModifiers: the destructor, then the
// constructor), every day for some countries. Each rebuild of an overlord gives each subject a new
// CStaticModifier (operator new, CalcModifiersFromSubjects) in ModifiersFromSubjectToOverlord,
// and the destructor frees only that array: lab, about 2 MB per year of play without mods,
// so a long campaign kept the game's memory growing even without a return. Nothing else
// points to them but the _Added list of the same object's total modifier (copies of a CModifier
// do not take that list), which the same destructor ends. So the detour deletes them first,
// with slot 0 and 1 as the game deletes a modifier, everywhere: they belong to this object only.
// They hold no game state; a rebuild makes new ones.
std::atomic<std::uint64_t> g_subject_modifiers_freed{0};

void on_country_modifiers_destroy(void* modifiers) noexcept {
    void** begin = at<void**>(modifiers, off::subject_modifiers_begin);
    void** end = at<void**>(modifiers, off::subject_modifiers_end);
    for (void** p = begin; p && p != end; ++p) {
        if (void* modifier = std::exchange(*p, nullptr)) {
            using DeleteFn = void* (*)(void* modifier, std::uint32_t flags);
            at<DeleteFn>(at<void*>(modifier, 0), 0)(modifier, 1);
            g_subject_modifiers_freed.fetch_add(1, std::memory_order_relaxed);
        }
    }
    g_country_modifiers_destroy.next(modifiers);
}

// Each CInstantTextBox makes a CPanGestureRecognizer (_pPanGesture), and only CGuiObject's
// destructor deletes recognizers: those on the box's gesture list. A box without a scrollbar
// (or a hidden one) takes it off that list, and the box's destructor leaves it, so every such
// box lost one (lab#39: 0.6-0.8 MB per campaign, 0.15 MB per Back). Off the list it gets no
// input and nothing else points to it; the box is its only observer. The detour deletes it,
// everywhere, before the box's own destructor runs, and only in the state the box left it:
// off the list, no fingers tracked (so not among CTouchGestures' active ones), no failure
// waiters, the box as its one observer, which is taken off first. Anything else stays as the
// game leaves it.
std::atomic<std::uint64_t> g_pan_gestures_freed{0};

bool owns_gesture(Bytes object, const void* gesture) noexcept {
    const Bytes begin = at<Bytes>(object, off::gui_gestures_begin);
    const Bytes end = at<Bytes>(object, off::gui_gestures_end);
    for (Bytes p = begin; p && p < end; p += off::gui_gesture_size)
        if (at<void*>(p, 0) == gesture) return true;
    return false;
}

bool pan_gesture_left_alone(Bytes box, Bytes pan) noexcept {
    if (owns_gesture(box, pan)) return false;  // ~CGuiObject deletes it
    if (at<void*>(pan, off::gesture_fingers_begin) != at<void*>(pan, off::gesture_fingers_end)) return false;
    if (at<void*>(pan, off::gesture_waiters_begin) != at<void*>(pan, off::gesture_waiters_end)) return false;
    void** observers = at<void**>(pan, off::gesture_observers_begin);
    void** observers_end = at<void**>(pan, off::gesture_observers_end);
    return observers && observers_end - observers == 1 && observers[0] == box + off::text_box_observer;
}

void on_text_box_destroy(void* object) noexcept {
    auto* box = static_cast<Bytes>(object);
    if (auto* pan = at<Bytes>(box, off::text_box_pan); pan && pan_gesture_left_alone(box, pan)) {
        at<void*>(box, off::text_box_pan) = nullptr;
        at<void**>(pan, off::gesture_observers_end) = at<void**>(pan, off::gesture_observers_begin);
        // Slot 0 with 1, as ~CGuiObject deletes a recognizer it owns.
        using DeleteFn = void* (*)(void* gesture, std::uint32_t flags);
        at<DeleteFn>(at<void*>(pan, 0), 0)(pan, 1);
        g_pan_gestures_freed.fetch_add(1, std::memory_order_relaxed);
    }
    g_text_box_destroy.next(object);
}

// The ruler, heir and consort of a country that has none are three objects the game
// makes once per process (CNullMonarch, CNullHeir, CNullQueen) and never resets, and
// effects on a country without one write into them: set_heir_culture on a country
// without an heir sets the culture of every such country's heir. So a campaign after a
// return started with what the last one wrote (lab#150: Anbennar's racial_misc.10 left
// tamran in the null heir, and the next campaign took another path on 19.11.1444
// than a fresh start, which has noculture there). Their culture, religion and dynasty
// are kept from the first main menu and put back after each reset, with their flags
// cleared; the objects stay where they are, as the game compares heirs with them.
struct NullCharacter {
    const char* name;
    std::uintptr_t* global = nullptr;
    Bytes object = nullptr;  // as the first main menu saw it
    void* religion = nullptr;
    void* culture = nullptr;
    void* dynasty = nullptr;
};
NullCharacter g_null_characters[] = {{"ruler"}, {"heir"}, {"consort"}};

std::string_view culture_name(const void* culture) noexcept {
    return culture ? game_string(static_cast<const std::uint8_t*>(culture) + off::culture_name) : "none";
}

// From the frontend's first ticks until a campaign starts: the state of a fresh process.
void keep_null_characters() {
    for (auto& c : g_null_characters) {
        if (c.object) continue;
        Bytes o = reinterpret_cast<Bytes>(*c.global);
        if (!o) continue;
        c.object = o;
        c.religion = at<void*>(o, off::monarch_religion);
        c.culture = at<void*>(o, off::monarch_culture);
        c.dynasty = at<void*>(o, off::monarch_dynasty);
        host().info("null {} kept: culture {}, religion {}, dynasty {}", c.name, culture_name(c.culture), c.religion,
                    c.dynasty);
    }
}

void restore_null_characters() {
    for (auto& c : g_null_characters) {
        Bytes o = reinterpret_cast<Bytes>(*c.global);
        if (!o) continue;
        if (o != c.object) {
            host().warn("null {} not put back: not the object the first main menu had", c.name);
            continue;
        }
        Bytes flags = o + off::monarch_flags;
        const auto flag_count = (at<Bytes>(flags, off::flags_end) - at<Bytes>(flags, off::flags_begin)) / 2;
        const std::string before(culture_name(at<void*>(o, off::monarch_culture)));
        const bool changed = at<void*>(o, off::monarch_culture) != c.culture ||
                             at<void*>(o, off::monarch_religion) != c.religion ||
                             at<void*>(o, off::monarch_dynasty) != c.dynasty || flag_count != 0;
        at<void*>(o, off::monarch_religion) = c.religion;
        at<void*>(o, off::monarch_culture) = c.culture;
        at<void*>(o, off::monarch_dynasty) = c.dynasty;
        game(sym::integer_flags_clear, flags);
        if (changed)
            host().info("null {} put back: culture {} -> {}, {} flags cleared", c.name, before, culture_name(c.culture),
                        flag_count);
    }
}

// Switches only the console (infinite_combat, the fixed dice, disable_dlc, run_year) or a
// tutorial (instant buildings and colonies) sets. The simulation reads them on every battle,
// siege, DLC check or building, and nothing resets them: a fresh process starts from the
// image's value, and a return kept what the last campaign left (lab, 18 November 2026: the
// console's infinite_combat was still on in the next campaign). Kept from the first main menu
// and put back before the reset, as CCurrentGameState::Clear reads disable_dlc.
struct GameSwitch {
    const char* name;
    std::int32_t* int_value = nullptr;
    std::uint8_t* byte_value = nullptr;
    hamweard::KeptSwitch kept;

    std::int32_t read() const noexcept { return int_value ? *int_value : *byte_value; }
    void write(std::int32_t v) noexcept {
        if (int_value) *int_value = v;
        else *byte_value = static_cast<std::uint8_t>(v);
    }
};
GameSwitch g_switches[] = {{"fixed dice"}, {"infinite_combat"}, {"disable_dlc"}, {"instant build"},
                           {"instant colony build"}, {"run_year"}};

void keep_switches() {
    for (auto& s : g_switches) s.kept.take(s.read());
}

void restore_switches() {
    for (auto& s : g_switches) {
        const std::int32_t now = s.read();
        if (!s.kept.differs(now)) continue;
        s.write(s.kept.value());
        host().info("{} put back: {} -> {}", s.name, now, s.kept.value());
    }
}

// A country's delayed event (an event with days = N) is a CDelayedEvent in _DelayedEvents.
// When its days run out, CCountry::DailyUpdateSerial fires it and takes it out of the vector,
// but never deletes it: only ~CCountry deletes the ones still waiting. TriggerEvent copies the
// scope it needs (AddPendingEvent), so nothing points to a fired one any more (lab: with
// Anbennar, whose start-up events wait a few days, about 2 MB per campaign). The game keeps
// running unchanged: right after that loop the plugin only notes the events it fired and took
// out, and deletes them, as ~CCountry deletes an event, once the world they came from is gone:
// after the reset of a return and after the load of a save.
std::mutex g_fired_mutex;
std::vector<void*> g_fired_events;
void* g_delayed_event_vtable = nullptr;

void note_fired_events(const anvil::Cpu& cpu) noexcept {
    auto* country = reinterpret_cast<Bytes>(cpu.r14);
    auto* frame = reinterpret_cast<Bytes>(cpu.rbp);
    const std::int32_t count = at<std::int32_t>(frame, off::fired_count);
    if (count <= 0) return;
    void* const* fired = at<void**>(frame, off::fired_events);
    void** begin = at<void**>(country, off::country_delayed_begin);
    void** end = at<void**>(country, off::country_delayed_end);
    std::lock_guard lock(g_fired_mutex);
    for (std::int32_t i = 0; i < count; ++i) {
        void* event = fired[i];
        // Only one the loop took out: one still in the vector is ~CCountry's to delete.
        if (event && std::find(begin, end, event) == end) g_fired_events.push_back(event);
    }
}

std::size_t free_fired_events() {
    std::vector<void*> events;
    {
        std::lock_guard lock(g_fired_mutex);
        events.swap(g_fired_events);
    }
    std::sort(events.begin(), events.end());
    events.erase(std::unique(events.begin(), events.end()), events.end());
    std::size_t freed = 0;
    for (void* event : events) {
        if (at<void*>(event, 0) != g_delayed_event_vtable) continue;  // not a CDelayedEvent any more
        // Slot 0 with 1, as the game deletes one: the scope's destructor, then the 0xF0 bytes.
        using DeleteFn = void* (*)(void* event, std::uint32_t flags);
        at<DeleteFn>(g_delayed_event_vtable, 0)(event, 1);
        ++freed;
    }
    return freed;
}

// A battle of a war is a CBattleResult that two things point to: the war's _BattleResults
// (CWar::AddBattleResult, or RecalculateCachedBattleResults after a load) and the
// CBattleResultChange of the war's history (+0x40), which the history's reader makes for
// each battle = { ... } of a save or of history/wars. ~CWar frees only the list, and the
// change's deleting destructor (shared with the other 0x48-byte changes) only the change,
// so every battle of every war was lost whenever the world went: at each reset and at each
// load of a save (lab, 19 November 2026: 1.6 MB in 5333 battles per return after a 1719
// save). The change is the one owner left, as the war it belongs to goes in the same
// teardown (its history is destroyed before its list, which ~CWar frees without reading).
// So while the game resets the world or loads a save, the change's slot deletes the battle
// first, as the game deletes a CBattleResult (slot 0 with 1); in the rest of the game it
// stays as it was.
using DeleteFn = void*(void* object, std::uint32_t flags);
anvil::PointerSwap<DeleteFn> g_battle_change_delete;
void* g_battle_result_vtable = nullptr;
std::atomic<int> g_world_teardown{0};  // inside ResetGame or the load of a save
std::atomic<std::uint64_t> g_battles_freed{0};

constexpr std::ptrdiff_t battle_change_result = 0x40;  // CBattleResultChange's CBattleResult*

class TeardownWindow {
public:
    TeardownWindow() { g_world_teardown.fetch_add(1, std::memory_order_relaxed); }
    ~TeardownWindow() { g_world_teardown.fetch_sub(1, std::memory_order_relaxed); }
    TeardownWindow(const TeardownWindow&) = delete;
    TeardownWindow& operator=(const TeardownWindow&) = delete;
};

void* on_battle_change_delete(void* change, std::uint32_t flags) noexcept {
    if (g_world_teardown.load(std::memory_order_relaxed) > 0) {
        auto& slot = at<void*>(change, battle_change_result);
        if (void* result = slot; result && at<void*>(result, 0) == g_battle_result_vtable) {
            slot = nullptr;
            at<DeleteFn*>(g_battle_result_vtable, 0)(result, 1);
            g_battles_freed.fetch_add(1, std::memory_order_relaxed);
        }
    }
    return g_battle_change_delete.previous()(change, flags);
}

// The rulers, heirs and consorts of a save's country history. The history's reader makes
// one CMonarch, CHeir or CQueen for each monarch = { ... }, heir = { ... } or queen = { ... }
// and gives it to the change (CMonarchChange +0x40..+0x58, CHeirChange and CQueenChange
// +0x40/+0x48); when the change runs, the country points to the same object. Only the
// characters made while the game reads history/ (and those made in play) go into
// CMonarchStore::_HistoricalMonarchs, the list DestroyDatabase deletes; ~CCountry and the
// changes' destructors delete none, so the save's characters were lost at every reset (lab,
// 19 November 2026: about 3 MB per return after a 1719 save, with their ID table nodes).
// ResetGame destroys the countries with the old game state, then calls DestroyDatabase,
// then replaces the ID tables (DeleteReferenceObjects). So while Hamweard resets the world,
// the changes' slots note their characters, and Hamweard deletes them as DestroyDatabase
// starts: the tables are still the ones the characters are in (~CMonarch erases its ID),
// and the world that could point to them is gone. A character is deleted only if it is a
// CMonarch, CHeir or CQueen by its exact vtable, not in the store, not a null character,
// and not deleted by anyone else since it was noted (the characters' own slots watch).
anvil::PointerSwap<DeleteFn> g_monarch_change_delete;
anvil::PointerSwap<DeleteFn> g_heir_change_delete;
anvil::PointerSwap<DeleteFn> g_queen_change_delete;
anvil::PointerSwap<DeleteFn> g_monarch_delete;
anvil::PointerSwap<DeleteFn> g_heir_delete;
anvil::PointerSwap<DeleteFn> g_queen_delete;
void* g_character_vtables[3] = {};  // CMonarch, CHeir, CQueen
std::uintptr_t* g_monarch_store = nullptr;
std::atomic<bool> g_noting_characters{false};
std::mutex g_noted_characters_mutex;
std::unordered_set<void*> g_noted_characters;
std::unordered_set<void*> g_deleted_characters;

constexpr std::ptrdiff_t change_characters = 0x40;  // the first character pointer of each change

void note_characters(void* change, int count) noexcept {
    if (!g_noting_characters.load(std::memory_order_relaxed)) return;
    const std::lock_guard lock(g_noted_characters_mutex);
    for (int i = 0; i < count; ++i)
        if (void* character = at<void*>(change, change_characters + 8 * i);
            character && !g_deleted_characters.contains(character))  // a change can outlive its character
            g_noted_characters.insert(character);
}

void forget_character(void* character) noexcept {
    if (!g_noting_characters.load(std::memory_order_relaxed)) return;
    const std::lock_guard lock(g_noted_characters_mutex);
    g_noted_characters.erase(character);
    g_deleted_characters.insert(character);
}

void* on_monarch_change_delete(void* change, std::uint32_t flags) noexcept {
    note_characters(change, 4);  // _pNew, _pOld, _pOldHeir, _pOldQueen
    return g_monarch_change_delete.previous()(change, flags);
}
void* on_heir_change_delete(void* change, std::uint32_t flags) noexcept {
    note_characters(change, 2);  // _pNew, _pOld
    return g_heir_change_delete.previous()(change, flags);
}
void* on_queen_change_delete(void* change, std::uint32_t flags) noexcept {
    note_characters(change, 2);
    return g_queen_change_delete.previous()(change, flags);
}
void* on_monarch_delete(void* character, std::uint32_t flags) noexcept {
    forget_character(character);
    return g_monarch_delete.previous()(character, flags);
}
void* on_heir_delete(void* character, std::uint32_t flags) noexcept {
    forget_character(character);
    return g_heir_delete.previous()(character, flags);
}
void* on_queen_delete(void* character, std::uint32_t flags) noexcept {
    forget_character(character);
    return g_queen_delete.previous()(character, flags);
}

struct CharactersFreed {
    std::size_t freed = 0, in_store = 0, other = 0;
};
CharactersFreed g_characters_freed;  // by the last reset

// No one deletes a leader at a reset: ~CCountry frees its leader list's nodes only, ~CUnit
// detaches its leader, ~CMercenaryCompany keeps its own (+0x48), a save's characters hold
// theirs only by ID (CMonarch::_LeaderID) and ~CMonarch frees none, and CCountry::KillLeader
// deletes a dead leader only for one or two tags (the player's, it seems). After DestroyDatabase,
// DeleteReferenceObjects takes every leader still in the table out of it
// (CReferenceObject::Destroy) without deleting it, so every leader of the old campaign was lost
// at each return (lab, 21 November 2026: about 3,400 per return after a 1719 save or a new
// campaign, mostly mercenaries, and 900 more of the save's characters). As DestroyDatabase
// starts, the world is gone and the leader ID table is still the old one: what it holds
// belongs to nothing left. The one destructor that deletes a leader after that point, the
// country history's ~CLeaderChange, finds it by ID first. So Hamweard deletes every plain
// CLeader still in the table, through its own destructor, after clearing the two links
// ~CLeader would follow into the gone world (its country by _CountryTag, its unit), so that
// the destructor only frees the leader's strings and erases its ID.
void* g_leader_vtable = nullptr;
const std::uintptr_t* g_static_id_tables = nullptr;  // CReferenceObject::_StaticIdHashTables
constexpr std::ptrdiff_t leader_unit = 0x58;       // CUnit* _pUnit
constexpr std::ptrdiff_t leader_mercenary = 0x55;  // bool _IsMercenary
constexpr std::ptrdiff_t leader_tag_valid = 0x77;  // the valid flag of CCountryTag _CountryTag (+0x70)

struct LeadersFreed {
    std::size_t freed = 0, mercenaries = 0, other = 0;
};
LeadersFreed g_leaders_freed;  // by the last reset

LeadersFreed free_old_leaders() {
    LeadersFreed n;
    for (Bytes leader : hamweard::id_table_objects(g_static_id_tables, hamweard::leader_id_type)) {
        if (at<void*>(leader, 0) != g_leader_vtable) {  // the null leader or another kind
            ++n.other;
            continue;
        }
        if (at<std::uint8_t>(leader, leader_mercenary)) ++n.mercenaries;
        at<void*>(leader, leader_unit) = nullptr;
        at<std::uint8_t>(leader, leader_tag_valid) = 0;
        at<DeleteFn*>(g_leader_vtable, 0)(leader, 1);
        ++n.freed;
    }
    return n;
}

// Ends the noting; run as DestroyDatabase starts inside Hamweard's reset.
CharactersFreed free_noted_characters() {
    std::unordered_set<void*> noted;
    {
        const std::lock_guard lock(g_noted_characters_mutex);
        g_noting_characters.store(false, std::memory_order_relaxed);
        noted.swap(g_noted_characters);
        g_deleted_characters.clear();
    }
    CharactersFreed n;
    if (noted.empty()) return n;
    auto* const* begin = reinterpret_cast<void* const*>(g_monarch_store[0]);
    auto* const* end = reinterpret_cast<void* const*>(g_monarch_store[1]);
    const std::unordered_set<void*> store(begin, end);
    for (void* character : noted) {
        if (store.contains(character)) {
            ++n.in_store;
            continue;
        }
        const bool null_character = std::any_of(std::begin(g_null_characters), std::end(g_null_characters),
                                                [&](const auto& c) { return *c.global == reinterpret_cast<std::uintptr_t>(character); });
        void* vtable = at<void*>(character, 0);
        if (null_character || std::find(std::begin(g_character_vtables), std::end(g_character_vtables), vtable) ==
                                  std::end(g_character_vtables)) {
            ++n.other;
            continue;
        }
        at<DeleteFn*>(vtable, 0)(character, 1);
        ++n.freed;
    }
    return n;
}

// The AI's army agents (CAIArmy, 0xBF8 bytes with their lists and maps) are held only by
// their king's _ArmyAgents, or its _KillList once cancelled; ~CAIKing frees both lists'
// nodes and never the agents (lab, 19 November 2026: about 1.1 MB per return after a 1719
// save). Kings die in CCurrentGameState::ClearAIStuff, which the old game state's teardown
// runs while the countries still live and after their units are deleted: CGameState::
// DeleteUnit detaches a unit from its agent (DetachArmy) first, so an agent's unit list
// holds live units only. So while Hamweard resets the world, the king's slot copies its
// army agents, lets the king die (its requests' destructors only free themselves), and then
// deletes each agent through its own destructor, which unlinks its units and the country's
// units that point to it and leaves the ID table, still the old one. An agent is deleted
// only if its vtable is exactly CAIArmy's, its owner is that king, and it is not one of the
// king's child agents (which ~CAIAgent deletes itself).
anvil::PointerSwap<DeleteFn> g_ai_king_delete;
void* g_ai_army_vtable = nullptr;
std::atomic<bool> g_freeing_agents{false};
std::atomic<std::uint32_t> g_agents_freed{0}, g_agents_kept{0};

constexpr std::ptrdiff_t agent_owner = 0x38;      // CAIAgent::_pOwner
constexpr std::ptrdiff_t agent_children = 0x40;   // CAIAgent::_AgentList
constexpr std::ptrdiff_t king_kill_list = 0xAC8;  // CAIKing::_KillList
constexpr std::ptrdiff_t king_army_agents = 0xC50;  // CAIKing::_ArmyAgents

// A CList's nodes: the element at +0, the next node at +0x10; the list's head at +0.
template <class F> void for_each_in_list(void* list, F&& f) {
    for (void* node = at<void*>(list, 0); node; node = at<void*>(node, 0x10)) f(at<void*>(node, 0));
}

void* on_ai_king_delete(void* king, std::uint32_t flags) noexcept {
    if (!g_freeing_agents.load(std::memory_order_relaxed)) return g_ai_king_delete.previous()(king, flags);
    std::unordered_set<void*> children;
    for_each_in_list(static_cast<Bytes>(king) + agent_children, [&](void* a) { children.insert(a); });
    std::vector<void*> agents;
    std::unordered_set<void*> seen;
    std::uint32_t kept = 0;
    auto take = [&](void* agent) {
        if (!agent || !seen.insert(agent).second) return;
        if (children.contains(agent) || at<void*>(agent, 0) != g_ai_army_vtable || at<void*>(agent, agent_owner) != king) {
            ++kept;
            return;
        }
        agents.push_back(agent);
    };
    for_each_in_list(static_cast<Bytes>(king) + king_army_agents, take);
    for_each_in_list(static_cast<Bytes>(king) + king_kill_list, take);
    void* result = g_ai_king_delete.previous()(king, flags);
    for (void* agent : agents) at<DeleteFn*>(g_ai_army_vtable, 0)(agent, 1);
    g_agents_freed.fetch_add(static_cast<std::uint32_t>(agents.size()), std::memory_order_relaxed);
    g_agents_kept.fetch_add(kept, std::memory_order_relaxed);
    return result;
}

// Windows whose owner dies without destroying them. They stayed hidden in the
// GUI root, seven per return in the menu (lab, 19 November 2026, Tongs ui tree):
// - the goto box (CGotoBox: of the main menu's game setup view and of the in-game
//   idler) and the tutorial's minimized button (CTutorial: of the main menu's
//   tutorial screen): their deleting destructors leave their window;
// - the right-click menus of the main menu (CFrontEnd) and of the in-game idler,
//   whose destructors do not read the field;
// - the start game dialog of the main menu's game setup view (CGameSetup), which
//   ~CGameSetup never frees, with its window and its ironman save selection
//   (start_game_dialog and IronmanSaveSelectDialog); both stay in the GUI reload list.
// Nothing else holds them: each window is created into that one field and the
// dialog only by CGameSetup's constructor. The swapped slots below only note
// them, since an owner can also die at exit; the next return, where Hamweard
// destroys the campaign's windows, frees them.
anvil::PointerSwap<DeleteFn> g_goto_box_delete;
anvil::PointerSwap<DeleteFn> g_tutorial_delete;
anvil::PointerSwap<DeleteFn> g_game_setup_delete;
anvil::PointerSwap<DeleteFn> g_frontend_delete;

void note_orphan(void* owner, std::ptrdiff_t field, std::string_view name) {
    if (void* window = at<void*>(owner, field)) {
        std::lock_guard lock(g_orphans_mutex);
        g_orphan_windows.insert_or_assign(window, name);
    }
}

void* on_goto_box_delete(void* box, std::uint32_t flags) noexcept {
    note_orphan(box, off::goto_box_window, "goto_box");
    return g_goto_box_delete.previous()(box, flags);
}

void* on_tutorial_delete(void* tutorial, std::uint32_t flags) noexcept {
    note_orphan(tutorial, off::tutorial_minimized, "tutorial_minimized");
    return g_tutorial_delete.previous()(tutorial, flags);
}

// The slot of CGameSetup's CFrontEndView vtable: `view` is that base, which
// the main menu's view list holds and deletes.
void* on_game_setup_delete(void* view, std::uint32_t flags) noexcept {
    Bytes setup = static_cast<Bytes>(view) - off::game_setup_view;
    if (void* dialog = at<void*>(setup, off::game_setup_start_dialog);
        dialog && at<void*>(dialog, 0) == g_start_dialog_vtable) {
        std::lock_guard lock(g_orphans_mutex);
        g_orphan_dialogs.insert(dialog);
    }
    return g_game_setup_delete.previous()(view, flags);
}

void* on_frontend_delete(void* frontend, std::uint32_t flags) noexcept {
    note_orphan(frontend, g_layout.frontend_right_click, "rightClickMenu");
    return g_frontend_delete.previous()(frontend, flags);
}

// At a return, while the in-game idler lives: its right-click menu, and the
// windows of its goto box and tutorial, should their objects outlive it.
void note_idler_orphans(Bytes idler) {
    note_orphan(idler, g_layout.idler_right_click, "rightClickMenu");
    if (void* box = at<void*>(idler, g_layout.idler_goto_box); box && at<void*>(box, 0) == g_goto_box_vtable)
        note_orphan(box, off::goto_box_window, "goto_box");
    if (void* tutorial = at<void*>(idler, g_layout.idler_tutorial); tutorial && at<void*>(tutorial, 0) == g_tutorial_vtable)
        note_orphan(tutorial, off::tutorial_minimized, "tutorial_minimized");
}

// Clears the AI's strength factor caches as DailyUpdateAI does at the end of each AI day. The
// AI fills them only inside its day, but the AI army map mode (console `mapmode armyeval`) and
// its province tooltip fill them on the main thread from the selected army's agent, and those
// entries stay until the next AI day: keyed by units and provinces the reset frees, whose
// addresses the next campaign's world can get again.
// The log tells a clear that ran (with its count, often 0) from one that did not.
void clear_strength_caches() {
    void* mutex = reinterpret_cast<void*>(*g_strength_cache_mutex);
    if (!mutex) {
        host().info("strength factor caches: their mutex is not made yet (no cache asked for), nothing to clear");
        return;
    }
    if (const int locked = game(sym::mutex_lock, mutex); locked != 0) {
        host().warn("strength factor caches not cleared: their mutex could not be locked (_Mtx_lock {})", locked);
        return;
    }
    std::uint64_t entries = 0;
    for (Bytes cache : hamweard::strength_caches(reinterpret_cast<const std::uint8_t*>(g_strength_cache_list))) {
        if (!cache) continue;
        entries += hamweard::strength_cache_entries(cache);
        game(sym::unit_strength_cache_clear, cache + hamweard::strength_cache_maps[0]);
        game(sym::province_strength_cache_clear, cache + hamweard::strength_cache_maps[1]);
        game(sym::province_strength_cache_clear, cache + hamweard::strength_cache_maps[2]);
    }
    game(sym::mutex_unlock, mutex);
    host().info("strength factor caches: {} entries of the last campaign cleared", entries);
}

void reset_world(Bytes app) {
    const std::size_t idea_groups = free_idea_group_flags();
    const std::size_t estate_values = free_estate_values();
    g_else_branches_freed.store(0, std::memory_order_relaxed);
    g_characters_freed = {};
    g_leaders_freed = {};
    {
        FreeElseWindow window;
        TeardownWindow teardown;
        g_noting_characters.store(true, std::memory_order_relaxed);
        g_agents_freed.store(0, std::memory_order_relaxed);
        g_agents_kept.store(0, std::memory_order_relaxed);
        g_freeing_agents.store(true, std::memory_order_relaxed);
        game(sym::reset_game, app, false);
        g_freeing_agents.store(false, std::memory_order_relaxed);
    }
    if (g_noting_characters.load(std::memory_order_relaxed)) {
        // DestroyDatabase did not run: keep the characters rather than delete them late.
        const std::lock_guard lock(g_noted_characters_mutex);
        g_noting_characters.store(false, std::memory_order_relaxed);
        host().warn("{} historical characters kept: the reset did not destroy the history", g_noted_characters.size());
        g_noted_characters.clear();
        g_deleted_characters.clear();
    }
    restore_null_characters();
    clear_strength_caches();
    const std::size_t fired = free_fired_events();
    host().info("memory the reset would lose: {} idea group tables, {} estate modifiers, {} else branches freed, {} fired delayed events freed, {} historical characters freed ({} in the store, {} others kept), {} leaders freed ({} of them mercenaries, {} others kept), {} AI army agents freed ({} others kept); {} subject modifiers, {} text box pan gestures and {} battle results freed since the last reset",
                idea_groups, estate_values, g_else_branches_freed.load(std::memory_order_relaxed), fired, g_characters_freed.freed,
                g_characters_freed.in_store, g_characters_freed.other,
                g_leaders_freed.freed, g_leaders_freed.mercenaries, g_leaders_freed.other,
                g_agents_freed.load(std::memory_order_relaxed), g_agents_kept.load(std::memory_order_relaxed),
                g_subject_modifiers_freed.exchange(0, std::memory_order_relaxed),
                g_pan_gestures_freed.exchange(0, std::memory_order_relaxed),
                g_battles_freed.exchange(0, std::memory_order_relaxed));
}

void on_history_move(void* history, const void* date) noexcept {
    const std::uint32_t before = g_else_branches_freed.load(std::memory_order_relaxed);
    {
        FreeElseWindow window;
        g_history_move.next(history, date);
    }
    host().info("new campaign: {} else branches of the history's effects freed",
                g_else_branches_freed.load(std::memory_order_relaxed) - before);
}

// Run by ResetGame (inside its own window), by the load of a save and by the
// game's exit; only the first two free the branches.
void on_history_destroy(void* history) noexcept {
    const bool in_reset = g_free_else.load(std::memory_order_relaxed) > 0;
    if (!in_reset && g_loading_save.load(std::memory_order_relaxed) <= 0) {
        g_history_destroy.next(history);
        return;
    }
    // In Hamweard's reset the old world is gone and the ID tables are still the old ones.
    if (in_reset && g_noting_characters.load(std::memory_order_relaxed)) g_characters_freed = free_noted_characters();
    if (in_reset) g_leaders_freed = free_old_leaders();  // after the characters: none of them deletes a leader
    const std::uint32_t before = g_else_branches_freed.load(std::memory_order_relaxed);
    {
        FreeElseWindow window;
        g_history_destroy.next(history);
    }
    if (!in_reset)
        host().info("a save loads: {} else branches of the old history freed",
                    g_else_branches_freed.load(std::memory_order_relaxed) - before);
}

void on_save_load_task(void* task) noexcept {
    g_loading_save.fetch_add(1, std::memory_order_relaxed);
    {
        TeardownWindow teardown;
        g_save_load_task.next(task);
    }
    g_loading_save.fetch_sub(1, std::memory_order_relaxed);
    // The load replaced the world the noted events came from.
    if (const std::size_t fired = free_fired_events()) host().info("a save loaded: {} fired delayed events freed", fired);
    if (const auto battles = g_battles_freed.exchange(0, std::memory_order_relaxed))
        host().info("a save loaded: {} battle results of the old world freed", battles);
}

// The frontend idler's first act after the hand-over: the outgoing idler is gone.
void on_frontend_restore(void* idler) {
    if (g_pending_menu_app && idler == g_pending_menu_idler) clear_province_graphics(g_pending_menu_app, idler);
    g_restore_device_objects.previous()(idler);
}

// The frontend idler's tick: the queued Back, then the deferred world reset.
void on_frontend_tick(void* idler, bool update) {
    if (!g_campaign_seen) {
        keep_null_characters();
        keep_switches();
    }
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
                             if (!*g_trigger_db) game(sym::trigger_db_create);
                             // A nation designed in the lobby lives outside the world, and only
                             // the start of a campaign clears it: left alone, it would join the
                             // next one.
                             game(sym::custom_countries_clear);
                             *g_custom_nation_used = 0;
                             g_history_reloading = true;
                             restore_switches();
                             reset_world(app);
                             validate_reloaded_history();  // a reset that did not walk the history
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
                             host().info("world reset (world {}): windows {} ms, reset {} ms; {}",
                                         static_cast<void*>(world()), ms(start, windows), ms(windows, Clock::now()),
                                         memory_figures());
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
    anvil::Change map_randomize;
    anvil::Change delayed_events_fired;

    void load(anvil::Host& h) override {
        g_world = h.data(sym::world);
        g_history = h.data(sym::history);
        g_trigger_db = h.data(sym::trigger_db);
        g_custom_nation_used = h.data(sym::custom_nation_used);
        g_map = h.data(sym::map);
        g_country_db = h.data(sym::country_db);
        g_strength_cache_list = h.data(sym::strength_cache_list);
        g_strength_cache_mutex = h.data(sym::strength_cache_mutex);
        g_null_characters[0].global = h.data(sym::null_monarch);
        g_null_characters[1].global = h.data(sym::null_heir);
        g_null_characters[2].global = h.data(sym::null_queen);
        g_switches[0].int_value = h.data(sym::fixed_dice);
        g_switches[1].byte_value = h.data(sym::infinite_combat);
        g_switches[2].byte_value = h.data(sym::disable_dlc);
        g_switches[3].byte_value = h.data(sym::instant_build);
        g_switches[4].byte_value = h.data(sym::instant_colony_build);
        g_switches[5].int_value = h.data(sym::run_year);
        const char* build = h.game().build_id;
        const auto layout = build ? hamweard::build_layout(build) : hamweard::BuildLayout::unknown;
        if (layout == hamweard::BuildLayout::unknown)
            throw anvil::Error(ANVIL_ERROR_UNSUPPORTED,
                               std::format("game build {}: Hamweard has no structure layout for it (only -steam-win64 "
                                           "and -gog-win64 builds); not loaded",
                                           build ? build : "unknown"));
        g_layout = layout == hamweard::BuildLayout::gog ? kGog : kSteam;
        g_minimap_vtable = h.address(sym::minimap_vtable);
        g_delayed_event_vtable = h.address(sym::delayed_event_vtable);
        g_battle_result_vtable = h.address(sym::battle_result_vtable);
        g_character_vtables[0] = h.address(sym::monarch_vtable);
        g_character_vtables[1] = h.address(sym::heir_vtable);
        g_character_vtables[2] = h.address(sym::queen_vtable);
        g_monarch_store = h.data(sym::monarch_store);
        g_leader_vtable = h.address(sym::leader_vtable);
        g_estate_value_vtable = h.address(sym::estate_value_vtable);
        g_static_id_tables = h.data(sym::static_id_tables);
        g_ai_army_vtable = h.address(sym::ai_army_vtable);
        g_start_dialog_vtable = h.address(sym::start_dialog_vtable);
        g_ironman_select_vtable = h.address(sym::ironman_select_vtable);
        g_goto_box_vtable = h.address(sym::goto_box_delete_slot);  // slot 0 is the vtable's start
        g_tutorial_vtable = h.address(sym::tutorial_delete_slot);
        // The GOG build has no Steam lobby: its four symbols are absent there.
        if (h.present(sym::steam_lobby_vtable)) {
            g_lobby_vtable = h.address(sym::steam_lobby_vtable);
            // The import CSteamLobby's destructor calls through (IAT 0x1B67730).
            if (HMODULE steam = GetModuleHandleW(L"steam_api64.dll"))
                g_unregister_call_result = reinterpret_cast<UnregisterCallResultFn*>(
                    reinterpret_cast<void*>(GetProcAddress(steam, "SteamAPI_UnregisterCallResult")));
            if (!g_unregister_call_result) h.warn("SteamAPI_UnregisterCallResult not found: a lobby search is not cancelled");
        }
        h.info("game build {}: {} layout, {}", build, hamweard::layout_name(layout),
               g_lobby_vtable ? "with the Steam lobby clean-up" : "no Steam lobby");

        // The three changes are enabled together after load(), or none.
        g_in_game_stock_path = static_cast<Bytes>(h.address(sym::in_game_menu_block)) + 7;
        g_pregame_stock_path = static_cast<Bytes>(h.address(sym::pregame_menu_block)) + 9;
        in_game = h.replace_block(sym::in_game_menu_block, [](anvil::Cpu& cpu) {
            if (const char* why = restart_reason()) {
                host().info("Exit to Menu {}: the game restarts", why);
                cpu.rcx = cpu.rbp + 0x240;  // the block's own lea
                cpu.rip = reinterpret_cast<std::uintptr_t>(g_in_game_stock_path);
                return;
            }
            return_to_menu(reinterpret_cast<Bytes>(cpu.r13));
        });
        pregame = h.replace_block(sym::pregame_menu_block, [](anvil::Cpu& cpu) {
            if (const char* why = restart_reason()) {
                host().info("Back {}: the game restarts", why);
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
        map_randomize = h.mid_hook(sym::map_randomize, [] {
            g_map_randomized = true;
            host().info("Random New World: the next return restarts the game");
        });
        delayed_events_fired = h.mid_hook(sym::delayed_events_fired, [](anvil::Cpu& cpu) { note_fired_events(cpu); });
        h.swap(g_tick, sym::frontend_tick_slot, &on_frontend_tick);
        h.swap(g_restore_device_objects, sym::frontend_restore_slot, &on_frontend_restore);
        h.swap(g_battle_change_delete, sym::battle_change_delete_slot, &on_battle_change_delete);
        h.swap(g_monarch_change_delete, sym::monarch_change_delete_slot, &on_monarch_change_delete);
        h.swap(g_heir_change_delete, sym::heir_change_delete_slot, &on_heir_change_delete);
        h.swap(g_queen_change_delete, sym::queen_change_delete_slot, &on_queen_change_delete);
        h.swap(g_monarch_delete, sym::monarch_vtable, &on_monarch_delete);
        h.swap(g_heir_delete, sym::heir_vtable, &on_heir_delete);
        h.swap(g_queen_delete, sym::queen_vtable, &on_queen_delete);
        h.swap(g_ai_king_delete, sym::ai_king_delete_slot, &on_ai_king_delete);
        h.swap(g_goto_box_delete, sym::goto_box_delete_slot, &on_goto_box_delete);
        h.swap(g_tutorial_delete, sym::tutorial_delete_slot, &on_tutorial_delete);
        h.swap(g_game_setup_delete, sym::game_setup_view_delete_slot, &on_game_setup_delete);
        h.swap(g_frontend_delete, sym::frontend_delete_slot, &on_frontend_delete);
        g_manager_global = reinterpret_cast<std::uintptr_t*>(h.game().image_base + h.value(sym::gui_manager_store, "manager"));
        g_registry_begin = static_cast<std::ptrdiff_t>(h.value(sym::widget_registry, "begin"));
        g_registry_end = static_cast<std::ptrdiff_t>(h.value(sym::widget_registry, "end"));
        h.hook(g_create_window, sym::gui_create_window, on_create_window);
        h.hook(g_history_execute, sym::history_execute, on_history_execute);
        h.hook(g_reset_game, sym::reset_game, on_reset_game);
        h.hook(g_multiplayer_click, sym::multiplayer_click, on_multiplayer_click);
        h.hook(g_multiplayer_availability, sym::multiplayer_availability, on_multiplayer_availability);
        h.hook(g_if_effect_delete, sym::if_effect_delete, on_if_effect_delete);
        h.hook(g_history_move, sym::history_move, on_history_move);
        h.hook(g_country_modifiers_destroy, sym::country_modifiers_destroy, on_country_modifiers_destroy);
        h.hook(g_history_destroy, sym::history_destroy, on_history_destroy);
        h.hook(g_save_load_task, sym::save_load_task, on_save_load_task);
        h.hook(g_text_box_destroy, sym::text_box_destroy, on_text_box_destroy);
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
