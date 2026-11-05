# Hamweard

[![build](https://github.com/happy-dwarf-team/hamweard/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/happy-dwarf-team/hamweard/actions/workflows/build.yml)
[![Latest release](https://img.shields.io/github/v/release/happy-dwarf-team/hamweard)](https://github.com/happy-dwarf-team/hamweard/releases/latest)
[![Steam Workshop](https://img.shields.io/badge/Steam%20Workshop-Hamweard-1b2838?logo=steam)](https://steamcommunity.com/sharedfiles/filedetails/?id=3832650792)
[![License: MIT](https://img.shields.io/github/license/happy-dwarf-team/hamweard)](LICENSE)

**Back to the main menu without restarting the game.** Hamweard is an
[Anvil](https://github.com/happy-dwarf-team/anvil) plugin for Europa Universalis IV 1.37.5
(Steam, and GOG 1.37.5.2, Windows x64). **Back** on the country selection screen and **Exit to
Menu** in a campaign take you to the main menu in a few seconds, instead of closing and
starting the game again.

**Install guide:
[anvil.happydwarf.dev/plugins/hamweard](https://anvil.happydwarf.dev/plugins/hamweard/).**

It is a port of [EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch) by VulonLok,
under the MIT License (see [Thanks](#thanks)).

**Status:** tested in the lab in single player, in ironman with the opt-in and with mods,
Anbennar among them. Most of that ran on 1.1.0, before the state reset fixes of 1.1.2, which
had their own runs (see [What was tested](#what-was-tested)). Multiplayer sessions are not
tested.

## Install and use

1. [Install Anvil](https://anvil.happydwarf.dev/docs/install/) 1.9 or newer in the game folder.
2. [Install the plugin](https://anvil.happydwarf.dev/docs/install-plugins/) from the **Catalog**
   tab of Anvil Manager, from the
   [Steam Workshop mod](https://steamcommunity.com/sharedfiles/filedetails/?id=3832650792), or
   from `hamweard-windows-x64.zip` in the
   [latest release](https://github.com/happy-dwarf-team/hamweard/releases/latest).
3. If you used the original EU4 Menu Patch, remove its DLL. Both change the same code, and Anvil
   verifies the bytes before it loads the plugin.
4. Start the game and approve the plugin when Anvil asks, or beforehand in Anvil Manager.

To go back to the main menu:

- In a campaign: **Exit to Menu** in the in-game menu, confirm, then **Quit** on the end-of-game
  screen.
- On the country selection screen: **Back**.

The main menu comes back in a few seconds, in the same process, and **Continue game** points at
your latest save, as after a restart. Without the plugin, or while Anvil keeps it suspended,
both buttons restart the game as usual.

## Limitations

- **Gameplay.** `affects_gameplay = true`: the next campaign starts in the same process
  instead of a fresh one, so anything the world reset leaves behind could change it. In
  the lab a save made after three returns matched a save from a fresh process, apart from
  the campaign id and the random seed. With Anbennar a new campaign after a return differed
  from one in a fresh process, even after a plain **Back** from the lobby, and the cause is
  not known yet.
- **Canals.** A canal changes the map itself, and the map outlives the campaign. The game's
  reset takes the canal's links out, but leaves other movement costs and naval distances
  behind. So the plugin copies them before the first canal and puts them back on a return.
  If they cannot be put back, the next return restarts the game, as it does without the
  plugin.
- **The Single Player list keeps a game bug.** Click a save in the list and start a new
  campaign from the same screen, and all province triggered modifiers are gone. The game
  does that too, with or without Hamweard. **Back** first clears it: the new campaign then
  matches one from a fresh start.
- **Multiplayer: off, and sessions not tested.** `allow_multiplayer = false`. In any multiplayer
  session, Exit to Menu and Back restart the game, as without the plugin. While a campaign with
  other players runs, Anvil takes the plugin's changes out. In a lobby, and as a host alone in
  the game, the plugin itself leaves by the game's restart.
- **Multiplayer after a campaign needs a restart.** The game marks a process that has entered
  single player as possibly corrupted and, with a Paradox session online, asks for a restart
  before multiplayer. A return keeps that mark. Offline the game does not check it: the
  Multiplayer button works and the Game Browser opens, a scan included (on GOG the button opens
  the Paradox login).
- **The Steam lobby clean-up has never run.** It is ported from the original, but no lobby was
  hosted or joined.
- **Ironman: off unless you allow it,** with **Allow in ironman** on the Manager's **Plugins**
  tab (the plugin declares `allow_ironman = "opt-in"`). With the opt-in it worked on Steam and
  GOG: the game saves when you leave and the save loads again. Without it the plugin is
  suspended and Exit to Menu restarts the game. An ironman save does not update the game's
  Continue game record, so after a return Continue game points at your last normal save. Load
  the ironman save from the Single Player list.
- **Mods: tested, not all of them.** No game file changes, so the checksum stays the same.
  Anbennar with Proper 2K UI and a playset of ten interface and graphics mods ran without
  trouble. A mod that does something unusual when a campaign ends may still behave
  differently.
- **Not seen in a test:** some message popups (an heir to name, a new colonial nation), the
  window of a lost game, the end of the game in 1821, custom nations and Random New World.
- **Memory grows by 5 to 15 MB per return,** and the menu stayed at full speed over 23
  returns in a row. About eight windows per campaign stay in the GUI root too (see
  [Windows a campaign leaves behind](#windows-a-campaign-leaves-behind)), but they are not the
  cause: the game's own reload in a campaign leaves 29 windows each
  time and memory stays flat. After many returns in one sitting, restart the game now and
  then.
- EU4 1.37.5 on Steam and the GOG build 1.37.5.2. On any other build Anvil does not load
  the plugin. The GOG build has no Steam lobby, so there is nothing to clean up there.

## Troubleshooting

Every return writes to `<EU4>/anvil/anvil.log`, in lines that start with `[hamweard]`. The
log says when the plugin chose a restart, and why.

| What you see | Why | What to do |
| --- | --- | --- |
| Exit to Menu or Back restarts the game | The plugin is not loaded, or Anvil suspended it | Open the Anvil panel (`Ctrl+Shift+F12`): is Hamweard loaded and approved? An ironman campaign needs the opt-in, in the panel or on the Manager's **Plugins** tab. |
| The same, and the log says `the map could not be put back` | A canal changed the map and the saved copy no longer fits it | Nothing to fix: the plugin keeps the restart for the rest of that sitting. |
| The same, and the log says `multiplayer` | A multiplayer session ends with a restart | Expected. |
| Anvil refuses to load Hamweard | The game is another build, or the original EU4 Menu Patch changed the same code | Use EU4 1.37.5 on Steam or GOG 1.37.5.2, and remove the Menu Patch DLL. |
| The game asks for a restart before multiplayer | The game's own rule after the single player screen, and a return keeps it | Restart the game once. |
| Continue game does not show your ironman campaign | The game does not record ironman saves there | Load it from the Single Player list. |
| Province triggered modifiers are gone in a new campaign | A game bug: a save was clicked in the list before the new campaign | Press **Back** after clicking a save, then start the campaign. |

A crash or anything odd after a return: open an
[issue](https://github.com/happy-dwarf-team/hamweard/issues) with `anvil.log`, and
`Documents/Paradox Interactive/Europa Universalis IV/logs/error.log`, from that sitting.

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

That is 12 campaigns without the hook, and earlier versions ran several more seven-campaign
series.

On the GOG build 1.37.5.2 (lab `docs/hamweard-gog.md`, 3 November 2026, Anvil with
`absent` symbols, vanilla profile, Steam-made saves): five campaigns of 30 days in one
process, five Exit to Menu and two Back, seven returns in 4.0 to 4.5 seconds each, no
crash and no warning in `anvil.log`.

More runs followed on 4 November 2026 (lab `docs/anvil-menu-patch.md`), with Hamweard
1.1.0 from the release, Anvil 1.9.0 with its overlay and log throttle, and Tongs 0.26.0:
66 returns in eight game runs, no crash, no `Should not be lazy loaded`, no popup left in
the GUI root after any return, and each run ended with the game's own Exit button or Exit
Game, or with a clean restart. 1.1.1 changes no code.

- **Saves after a return:** new games (not only loaded saves), Save Game, Load Game in the
  menu and in a running campaign, autosaves, and the same save loaded in a fresh process
  and after three returns: the two saved files differ only in the campaign id and the
  random seed.
- **Changing country:** the console's `tag` and observer mode, each followed by Exit to
  Menu and loading the save again.
- **Naval battles:** four campaigns with a war at sea and at least six end-of-naval-battle
  windows.
- **Multiplayer menu:** the Game Browser and a scan of the Internet list after a return,
  then Back. No hosting.
- **Ironman with the opt-in:** Steam and GOG, new campaigns after a return, two in a row,
  the save on leaving and loading it again. Without the opt-in: suspended, and Exit to Menu
  restarted the game.
- **Mods:** Anbennar with Proper 2K UI Enhanced (9 returns, returns in 5.5 to 6.6 seconds),
  and a copy of a ten-mod playset (8 returns, one campaign over a year long).
- **Bellows 0.19.0 and the Anvil overlay alongside:** 23 returns in a row, the menu at
  163 frames per second at the end.
- **GOG:** a save made on GOG, Continue game, ironman, a campaign of 400 days, 5 returns.

The state reset fixes of 1.1.2 were tested the same day. The same seed gave the same new
campaign byte by byte, the campaign
id apart, in a fresh process and after returns from campaigns started in 1444, 1453 and
1508, from a save of a 1508 start, from a 1519 save, and after Back with a later date or a
save picked in the list; on GOG after three returns as well. A ten-mod playset gave the same
result, Anbennar did not (see [Limitations](#limitations)). The menu's checksum stayed
the same.

Canals were tested on 5 November 2026: after a 1719 save with canals on Steam and on GOG, a
new campaign matched a fresh one, with no restart, 0 naval distances different and the
campaign id the only other difference.

The achievement list and the multiplayer check of 1.1.2 were tested that day too, with 27 returns in one process, 13 Exit to Menu and 14 Back, no restart and
no crash. After a return the list stayed empty, and a normal save had no `achievement`
block, also after an ironman save, a country change and a save that still has the block. An
ironman save loaded after each of those had its 52 achievements, and a new ironman
campaign said achievements can be earned.

Not tested: multiplayer sessions and the Steam lobby clean-up, and the windows and modes
listed under [Limitations](#limitations). The multiplayer check was read from the game's
code only: the Host a server window was opened and cancelled, nothing was hosted or joined.

## What it changes in the game

| Site | The game | The plugin |
| --- | --- | --- |
| `0x815CAE`, the Exit to Menu branch of `CInGameIdler::Idle` | sets the exit and restart flags: the game quits and starts again | in single player releases the campaign's leftovers, queues a new main menu and resumes at the stock epilogue; in a multiplayer session, or when a canal's map could not be put back, the game's restart |
| `0x111DFD7`, Back on the country selection screen | the same flags | in single player queues the switch for the menu's next tick; in a multiplayer lobby, or when a canal's map could not be put back, the game's restart |
| slot `+0x20` of the frontend idler vtable | the frontend tick | runs the queued switch, destroys the last campaign's windows, resets the world once the old idler is gone and forgets the achievement list the last loaded save carried, then the stock tick |
| slot `+0x30` of the frontend idler vtable | builds the menu, starting with the loading screen | clears the last campaign's map objects first, so unit banners do not stand on the loading screen |
| `0x78F250`, `ApplyCanalLogic` | rewrites the map's neighbours for a canal | the same, and before the first canal the plugin copies the map's links and naval distances, to put them back after the world reset |
| `0x7E8470`, `CHistoryDataBase::Execute` | walks the history between two dates | does nothing while a return waits for its world reset: the new menu would walk it on the old world, which crashed after a start from a later date |
| `0x14E9030` (GUI manager create), `0x10A0735` (popup window), `0x803270`, `0x81AF10` | create windows, build the campaign GUI | the same, and the plugin notes which windows belong to the campaign |

The world's `PotentialCorruption` flag stays as the game keeps it through its reset. Only
the `-nudge` reason (2), which the reset flattens to 1, is put back. Earlier versions, like
the original, cleared the flag after every return.

## Why a return takes a few seconds

A return took 3.5 to 4 seconds in the lab, and about 3 of those are the game's own world
reset (`ResetGame`). Almost all of that re-reads the databases and the history from disk,
which is what gives the next campaign a clean start, so the plugin does not skip it.
Loading a save from the menu is the game's own path, and Hamweard adds nothing to it.

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
duplicates of frontend names (`goto_box` and `rightClickMenu` twice each,
`start_game_dialog`, `tutorial_minimized`, `IronmanSaveSelectDialog`) and `dlc_entry`.

Earlier versions also had a mid hook on every sprite draw that loaded such a texture
first. With the popups destroyed it had nothing left to do, and the runs under
[What was tested](#what-was-tested) passed without it, so it was removed.

## Build

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DEU4_EXE="<EU4>/eu4.exe" -DEU4_DIR="<EU4>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target hamweard-check
```

The check target runs `anvil check --exe` on all 37 symbols and rewrites `symbols.lock`;
commit it with `symbols.toml`. `hamweard-deploy` (with `EU4_DIR`) copies the plugin into
the game folder.

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
[windows a campaign leaves behind](#windows-a-campaign-leaves-behind).

## Why "Hamweard"?

Old English *hāmweard* means "homeward": the plugin takes you from a campaign back to the
main menu as if back home, without a restart on the way. The rest of the team's tools are
named after the forge (Anvil, Bellows, Tongs, Etch); this one is the walk home after work.

## License

MIT, see [LICENSE](LICENSE). Hamweard keeps the original's notice, (c) 2026 VulonLok
([EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch)), next to the port's.
