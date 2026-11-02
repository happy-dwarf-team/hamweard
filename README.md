# Menu Patch for Anvil

An [Anvil](https://github.com/happy-dwarf-team/anvil) plugin for Europa Universalis IV
1.37.5 (Steam, Windows x64): **Back** on the country selection screen and **Exit to Menu**
in a campaign take you back to the main menu without restarting the game.

It is a port of [EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch) by
VulonLok (MIT), 0.1.4 at `7752efd`, which needs an EU4DLL-style `VERSION.dll` loader. The
research is the original's: where the game restarts, what it leaves behind (the Steam
lobby, the minimap controller, pending diplomatic notifications, the Continue game
record) and the fix for the crash when the same save is loaded again. The port replaces
the original's hand-written stubs, near allocations and page protection with Anvil's
`replace_block` and `swap_pointer`, and its own log with `anvil.log`.

**Status: prototype.** Tested in the lab on single-player campaigns only (seven campaigns
in one process, several times); multiplayer and ironman are not tested.

## What it changes in the game

| Site | The game | The plugin |
| --- | --- | --- |
| `0x815CAE`, the Exit to Menu branch of `CInGameIdler::Idle` | sets the exit and restart flags: the game quits and starts again | releases the campaign's leftovers, queues a new main menu and resumes at the stock epilogue |
| `0x111DFD7`, Back on the country selection screen | the same flags | queues the switch for the menu's next tick |
| slot `+0x20` of the frontend idler vtable | the frontend tick | runs the queued switch, resets the world once the old idler is gone, then the stock tick |

After the world reset it puts back the world's multiplayer restriction reason, which the
game's reset flattens to 1, once the menu owns the frame, the checksum is ready and the
Steam lobby is idle. Steam's own checks are left alone.

## Gameplay, ironman and multiplayer

- `affects_gameplay = true`: the next campaign starts in the same process instead of a
  fresh one, so anything the reset leaves behind could change it.
- Ironman: off unless the player allows it in Anvil (`allow_ironman = "opt-in"`).
- Multiplayer: never (`allow_multiplayer = false`). While a multiplayer or ironman session
  runs, Anvil takes the plugin's changes out and Exit to Menu restarts the game as usual.
- No game file changes, so the checksum stays the same.

## Install

Needs Anvil 1.5 or newer. Unpack the release zip into `<EU4>/anvil/plugins/` (it makes
`anvil/plugins/menu-patch/`). Remove the original EU4 Menu Patch DLL if you used it: both
would change the same code, and Anvil verifies the bytes before it loads the plugin.

## Build

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DEU4_EXE="<EU4>/eu4.exe" -DEU4_DIR="<EU4>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target menu_patch-check
```

The check target runs `anvil check --exe` on all 26 symbols and rewrites `symbols.lock`;
commit it with `symbols.toml`.

## What was tested

The runs are in the EU4 engine lab (`docs/anvil-menu-patch.md`). In one process: Back
from the country selection, Exit to Menu, loading the same save again, Continue game with
the refreshed record, and up to five returns in a row all reach the menu without a
restart, and `anvil-game` ends and starts its sessions. With the workaround, seven
campaigns (30 to 140 days at speed 3) ran in one process without a crash. Not tested: multiplayer and Steam
lobbies (hosting, joining, leaving), ironman with the opt-in, and mods.

## Windows a campaign leaves behind

1.37.5 never destroys the windows of a campaign: it restarts the process instead. The
plugin records the windows the GUI manager creates (`0x14E9030`) from the in-game idler's
constructor on, and destroys the ones still alive (checked in the widget registry, by
name) once the outgoing idler is gone; windows named like one of the fresh main menu's,
`load_screen` and `console_wnd` are kept. Message popups (`DefaultPopup`) do not go through
the GUI manager: `eu3dialog` (`0x10A06C0`) builds their window itself, so the plugin also
records the window it stores in the dialog (`0x10A0735`). Each window is hidden before it is
destroyed, as the HUD reload does. That removes about 140 to 160 windows per campaign, all
popups included. About 8 per campaign still stay, duplicates of frontend names (`goto_box`,
`rightClickMenu`, `start_game_dialog` and the like) and `dlc_entry`.

## The texture workaround

Each leaked popup keeps its settings button counted as shown, so the shown count of the
lazy (`loadType = "INGAME"`) sprite `MessageSettingsButton` never returns to 0 (in the lab
it equalled the number of leaked popups, 106 after seven campaigns). An icon loads a lazy
texture only when that count was 0, so after the texture had been released the next
campaign drew a freed handle: the game logged `Should not be lazy loaded` and crashed in
`d3d9.dll` (`SetTexture`) in the second to fourth campaign. A mid hook at the start of the
sprite draw (`0x156BFB0`), active after the first return, loads such a texture first, as the
first shown icon would have, and logs it. Since the popups are destroyed too, the count no
longer grows (11 to 19 at each Exit to Menu over seven campaigns) and the hook had nothing to
load; seven campaigns without it ran without a crash too. It stays as a safety net for paths
the lab did not try.

## License

MIT, see [LICENSE](LICENSE): the original's notice (c) 2026 VulonLok and the port's.
