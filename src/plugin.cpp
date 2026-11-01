// Menu Patch: Back on the country selection screen and Exit to Menu in a
// campaign return to the main menu instead of restarting the game.
//
// A port of EU4 Menu Patch 0.1.4 (https://github.com/YozoraTempest/EU4-Menu-Patch,
// MIT License, Copyright (c) 2026 VulonLok) to Anvil: the clean-up sequence,
// the deferred world reset and the multiplayer restore are the original's,
// the hand-written stubs, near allocations and page protection are Anvil's
// replace_block and swap_pointer. Everything runs on the game's main thread.
#include <anvil/anvil.hpp>

#include "menu_transition.hpp"
#include "symbols.gen.hpp"

#include <windows.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace {

using Bytes = std::uint8_t*;

template <class T> T& at(void* object, std::ptrdiff_t offset) noexcept {
    return *reinterpret_cast<T*>(static_cast<Bytes>(object) + offset);
}

// Structure offsets of 1.37.5, from EU4 Menu Patch.
namespace off {
constexpr std::ptrdiff_t app_current_idler = 0x40;
constexpr std::ptrdiff_t app_next_idler = 0x70;
constexpr std::ptrdiff_t app_exit = 0x83;
constexpr std::ptrdiff_t app_restart = 0x84;
constexpr std::ptrdiff_t app_checksum_ready = 0x328;
constexpr std::ptrdiff_t app_services = 0x348;  // its +0x10 is the Steam lobby
constexpr std::ptrdiff_t app_menu_arg_1 = 0x350;
constexpr std::ptrdiff_t app_menu_arg_2 = 0x358;

constexpr std::ptrdiff_t idler_app = 0x330;
constexpr std::ptrdiff_t idler_minimap = 0x10E0;
constexpr std::ptrdiff_t idler_exit_flag = 0x12E9;
constexpr std::ptrdiff_t idler_notifications = 0x1400;
constexpr std::ptrdiff_t menu_pending = 0x900;

constexpr std::ptrdiff_t lobby_id = 0x1C0;
constexpr std::ptrdiff_t lobby_hosting = 0x299;
constexpr std::ptrdiff_t lobby_flag_29b = 0x29B;
constexpr std::ptrdiff_t lobby_searching = 0x29C;
constexpr std::ptrdiff_t lobby_create_call = 0x2B0;
constexpr std::ptrdiff_t lobby_join_call = 0x2E0;
constexpr std::ptrdiff_t lobby_search_callback = 0x300;
constexpr std::ptrdiff_t lobby_search_call = 0x310;

constexpr std::ptrdiff_t world_multiplayer_reason = 0x23B4;

constexpr std::size_t main_menu_size = 0xB48;
}  // namespace off

using TickFn = void(void* idler, bool update);
using UnregisterCallResultFn = void(void* callback, std::uint64_t call);

anvil::PointerSwap<TickFn> g_tick;
std::uintptr_t* g_world = nullptr;    // the global holding the root object
std::uintptr_t* g_history = nullptr;  // the global holding the history
void* g_lobby_vtable = nullptr;
void* g_minimap_vtable = nullptr;
UnregisterCallResultFn* g_unregister_call_result = nullptr;

// Set by the pregame block, taken by the next frontend tick.
std::atomic<void*> g_pending_frontend_app{nullptr};
// Main thread only: the switch to the menu that is under way.
Bytes g_pending_menu_app = nullptr;
void* g_pending_menu_idler = nullptr;
std::uint32_t g_previous_multiplayer_reason = 0;
bool g_restore_deferral_logged = false;
std::uint32_t g_returns = 0;
menu_patch::WorldResetGate g_reset_gate;

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

std::uint32_t multiplayer_reason() noexcept {
    Bytes w = world();
    return w ? at<std::uint32_t>(w, off::world_multiplayer_reason) : 0;
}

Bytes steam_lobby(Bytes app) noexcept {
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

bool steam_session_idle(Bytes app) {
    Bytes lobby = steam_lobby(app);
    const bool idle = lobby && !at<std::uint64_t>(lobby, off::lobby_id) &&
                      !at<std::uint64_t>(lobby, off::lobby_create_call) &&
                      !at<std::uint64_t>(lobby, off::lobby_join_call) && !at<std::uint8_t>(lobby, off::lobby_hosting);
    if (!idle && !g_restore_deferral_logged) {
        host().info("multiplayer restore deferred: {}", lobby ? "the Steam lobby is busy" : "no Steam lobby service");
        g_restore_deferral_logged = true;
    }
    return idle;
}

// Once the new main menu owns the frame, has finished its first update and the
// checksum is ready, put back the multiplayer restriction ResetGame flattened
// (it turns any non-zero reason into 1). Steam's own checks are left alone.
void restore_multiplayer_access(void* idler) {
    if (idler != g_pending_menu_idler || !g_pending_menu_app) return;
    Bytes app = g_pending_menu_app;
    if (at<void*>(app, off::app_current_idler) != idler || at<void*>(app, off::app_next_idler) != nullptr ||
        at<std::uint32_t>(idler, off::menu_pending) != 0 || !at<std::uint8_t>(app, off::app_checksum_ready))
        return;
    Bytes w = world();
    if (!w) return;
    auto& reason = at<std::uint32_t>(w, off::world_multiplayer_reason);
    const std::uint32_t before = reason;
    if (before == 1 && g_previous_multiplayer_reason == 1 && !steam_session_idle(app)) return;
    if (before == 1 && g_previous_multiplayer_reason != 0)
        reason = g_previous_multiplayer_reason == 1 ? 0 : g_previous_multiplayer_reason;
    host().info("main menu active: multiplayer reason {} -> {} (before the return {}), checksum ready", before,
                reason, g_previous_multiplayer_reason);
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
    void* memory = game(sym::game_alloc, std::uint64_t{off::main_menu_size});
    void* menu = game(sym::main_menu_construct, memory, at<void*>(app, off::app_menu_arg_1),
                      at<void*>(app, off::app_menu_arg_2), app);
    // SetNextIdler takes the pointer and nulls the local.
    game(sym::set_next_idler, app, &menu, false);
    g_pending_menu_app = app;
    g_pending_menu_idler = at<void*>(app, off::app_next_idler);
    g_reset_gate.queue(app, g_pending_menu_idler);
    g_previous_multiplayer_reason = reason;
    g_restore_deferral_logged = false;
    ++g_returns;
    host().info("return to the main menu #{} queued (menu {}, exit {}, restart {})", g_returns, g_pending_menu_idler,
                at<std::uint8_t>(app, off::app_exit), at<std::uint8_t>(app, off::app_restart));
}

void release_minimap(Bytes idler) {
    auto& slot = at<void*>(idler, off::idler_minimap);
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

// In CInGameIdler::Idle, instead of setting exit + restart.
void return_to_menu(Bytes idler) {
    Bytes app = at<Bytes>(idler, off::idler_app);
    const std::uint32_t reason = multiplayer_reason();
    host().info("Exit to Menu: idler {}, world {}, multiplayer reason {}", static_cast<void*>(idler),
                static_cast<void*>(world()), reason);
    at<std::uint8_t>(idler, off::idler_exit_flag) = 0;
    BusyScope busy;
    game(sym::in_game_exit, idler);
    leave_steam_session(app);
    game(sym::campaign_exit_cleanup);
    // The in-game reload clears this owner before it replaces the world; only
    // its full destructor removes the GUI and the callbacks too.
    if (void* notices = at<void*>(idler, off::idler_notifications)) game(sym::notifications_destroy, notices);
    release_minimap(idler);
    construct_menu(app, reason);
}

// The frontend idler's tick: the queued Back, then the deferred world reset.
void on_frontend_tick(void* idler, bool update) {
    if (auto* app = static_cast<Bytes>(g_pending_frontend_app.exchange(nullptr))) {
        host().info("Back: returning from the country selection");
        const std::uint32_t reason = multiplayer_reason();
        BusyScope busy;
        leave_steam_session(app);
        construct_menu(app, reason);
        return;
    }
    if (Bytes app = g_pending_menu_app) {
        g_reset_gate.run(app, idler, at<void*>(app, off::app_current_idler), at<void*>(app, off::app_next_idler),
                         [&] {
                             BusyScope busy;
                             game(sym::reset_game, app, false);
                             // A frontend owns this reset, so the history needs the
                             // post-load validation ResetGame runs only for an in-game idler.
                             game(sym::history_post_validate, reinterpret_cast<void*>(*g_history));
                             host().info("world reset (world {})", static_cast<void*>(world()));
                         });
    }
    g_tick.previous()(idler, update);
    restore_multiplayer_access(idler);
}

struct MenuPatch : anvil::Plugin {
    anvil::Change in_game;
    anvil::Change pregame;

    void load(anvil::Host& h) override {
        g_world = h.data(sym::world);
        g_history = h.data(sym::history);
        g_lobby_vtable = h.address(sym::steam_lobby_vtable);
        g_minimap_vtable = h.address(sym::minimap_vtable);
        // The import CSteamLobby's destructor calls through (IAT 0x1B67730).
        if (HMODULE steam = GetModuleHandleW(L"steam_api64.dll"))
            g_unregister_call_result = reinterpret_cast<UnregisterCallResultFn*>(
                reinterpret_cast<void*>(GetProcAddress(steam, "SteamAPI_UnregisterCallResult")));
        if (!g_unregister_call_result) h.warn("SteamAPI_UnregisterCallResult not found: a lobby search is not cancelled");

        // The three changes are enabled together after load(), or none.
        in_game = h.replace_block(sym::in_game_menu_block, [](anvil::Cpu& cpu) {
            return_to_menu(reinterpret_cast<Bytes>(cpu.r13));
        });
        pregame = h.replace_block(sym::pregame_menu_block, [](anvil::Cpu& cpu) {
            g_pending_frontend_app.store(reinterpret_cast<void*>(cpu.rbx));
        });
        h.swap(g_tick, sym::frontend_tick_slot, &on_frontend_tick);
    }
};

}  // namespace

ANVIL_PLUGIN(MenuPatch)
