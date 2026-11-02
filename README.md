# Menu Patch for Anvil

An [Anvil](https://github.com/happy-dwarf-team/anvil) plugin for Europa Universalis IV
1.37.5 (Steam, Windows x64): **Back** on the country selection screen and **Exit to Menu**
in a campaign take you back to the main menu without restarting the game.

## Thanks

This is a port of [EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch) by
VulonLok (MIT), 0.1.4 at `7752efd`. The research is theirs: where the game restarts, what it
leaves behind (the Steam lobby, the minimap controller, pending diplomatic notifications,
the Continue game record) and the fix for the crash when the same save is loaded again.
Thank you for working all of that out and for publishing it under a licence that lets
others build on it.

The original needs an EU4DLL-style `VERSION.dll` loader. The port replaces its
hand-written stubs, near allocations and page protection with Anvil's `replace_block`,
`swap_pointer` and hooks, its own log with `anvil.log`, and adds the clean-up of the
windows a campaign leaves behind (below).

**Status: prototype.** Tested in the lab on single-player campaigns only; multiplayer,
ironman and mods are not tested.

## Install and use

1. Install [Anvil](https://anvil.happydwarf.dev/) 1.5 or newer in the game folder.
2. Unpack the release zip into `<EU4>/anvil/plugins/` (it makes
   `anvil/plugins/menu-patch/`). If you used the original EU4 Menu Patch, remove its DLL:
   both change the same code, and Anvil verifies the bytes before it loads the plugin.
3. Start the game and approve the plugin when Anvil asks (or beforehand in Anvil Manager).
4. In a campaign: **Exit to Menu** in the in-game menu, confirm, then **Quit** on the
   end-of-game screen. On the country selection screen: **Back**. Either way the main
   menu comes back in a few seconds, in the same process, and **Continue game** points at
   your latest save, as after a restart.

Without the plugin, or while Anvil keeps it suspended, both buttons restart the game as
usual.

## Limitations

- **Gameplay.** `affects_gameplay = true`: the next campaign starts in the same process
  instead of a fresh one, so anything the world reset leaves behind could change it. The
  plugin also writes the world's multiplayer restriction (below).
- **Multiplayer: not tested, and off.** `allow_multiplayer = false`: while a multiplayer
  session runs, Anvil takes the plugin's changes out and Exit to Menu restarts the game.
  The Steam lobby clean-up is ported from the original but was never exercised (hosting,
  joining or leaving a lobby).
- **Ironman: off unless you allow it** in Anvil (`allow_ironman = "opt-in"`). Not tested
  either way, including the save the game makes when you leave an ironman campaign.
- **Mods: not tested.** No game file changes, so the checksum stays the same.
- About eight windows per campaign stay in the GUI root after a return (see below). Their
  memory cost over many returns was not measured.
- EU4 1.37.5 Steam only. On any other build Anvil does not load the plugin.

## What it changes in the game

| Site | The game | The plugin |
| --- | --- | --- |
| `0x815CAE`, the Exit to Menu branch of `CInGameIdler::Idle` | sets the exit and restart flags: the game quits and starts again | releases the campaign's leftovers, queues a new main menu and resumes at the stock epilogue |
| `0x111DFD7`, Back on the country selection screen | the same flags | queues the switch for the menu's next tick |
| slot `+0x20` of the frontend idler vtable | the frontend tick | runs the queued switch, destroys the last campaign's windows and resets the world once the old idler is gone, then the stock tick |
| `0x14E9030` (GUI manager create), `0x10A0735` (popup window), `0x803270`, `0x81AF10` | create windows, build the campaign GUI | the same, and the plugin notes which windows belong to the campaign |

After the world reset it puts back the world's multiplayer restriction reason, which the
game's reset flattens to 1, once the menu owns the frame, the checksum is ready and the
Steam lobby is idle. Steam's own checks are left alone.

## Windows a campaign leaves behind

1.37.5 never destroys the windows of a campaign: it restarts the process instead. Left
alone, every campaign left about 90 top-level windows behind, message popups among them,
and their icons stayed counted as shown. A lazy (`loadType = "INGAME"`) sprite loads its
texture only when that count was 0, so after the texture had been released the next
campaign drew a freed handle: the game logged `Should not be lazy loaded` for
`MessageSettingsButton` (the settings button of message popups) and crashed in `d3d9.dll`
in the second to fourth campaign.

So the plugin records the windows the GUI manager creates (`0x14E9030`) from the in-game
idler's constructor on, and the window each message popup (`eu3dialog`, `0x10A06C0`)
builds for itself (`0x10A0735`). Once the outgoing idler is gone it destroys the ones
still alive (checked in the widget registry, by name), each hidden first, as the HUD
reload does. Windows named like one of the fresh main menu's, `load_screen` and
`console_wnd` are kept. That removes about 100 to 160 windows per campaign, all popups
included, and the shown count no longer grows. About eight per campaign still stay:
duplicates of frontend names (`goto_box`, `rightClickMenu`, `start_game_dialog` and the
like) and `dlc_entry`.

Earlier versions also had a mid hook on every sprite draw that loaded such a texture
first. With the popups destroyed it had nothing left to do, and the lab runs below passed
without it, so it was removed.

## What was tested

The runs are in the EU4 engine lab (`docs/anvil-menu-patch.md`), single player on 1.37.5
Steam with a vanilla profile. In one process: Back from the country selection, Exit to
Menu, loading the same save again, Continue game with the refreshed record, and seven
returns in a row reach the menu without a restart, and `anvil-game` ends and starts its
sessions. Without the sprite-draw hook:

- seven campaigns in a row (30 days each), seven Exit to Menu and two Back: no crash, no
  `Should not be lazy loaded`, no popup left in the GUI root after any return;
- five more campaigns in a fresh process, four of them with a war, a battle (the
  end-of-battle window) and a peace (the end-of-war window), three of them about two
  years long and one three years long without a war: the same result. The return to the
  menu took 3.5 to 4 seconds after each.

That is 12 campaigns with the current code, and earlier versions ran several more seven-campaign series.

Not tested: multiplayer and Steam lobbies, ironman with the opt-in, mods, naval battles,
and Bellows or the Anvil overlay running alongside.

## Build

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DEU4_EXE="<EU4>/eu4.exe" -DEU4_DIR="<EU4>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target menu_patch-check
```

The check target runs `anvil check --exe` on all 32 symbols and rewrites `symbols.lock`;
commit it with `symbols.toml`. `menu_patch-deploy` (with `EU4_DIR`) copies the plugin into
the game folder.

## License

MIT, see [LICENSE](LICENSE): the original's notice (c) 2026 VulonLok and the port's.
