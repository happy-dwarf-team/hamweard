# Hamweard

[![build](https://github.com/happy-dwarf-team/hamweard/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/happy-dwarf-team/hamweard/actions/workflows/build.yml)
[![Latest release](https://img.shields.io/github/v/release/happy-dwarf-team/hamweard)](https://github.com/happy-dwarf-team/hamweard/releases/latest)
[![Steam Workshop](https://img.shields.io/badge/Steam%20Workshop-Hamweard-1b2838?logo=steam)](https://steamcommunity.com/sharedfiles/filedetails/?id=3832650792)
[![License: MIT](https://img.shields.io/github/license/happy-dwarf-team/hamweard)](LICENSE)

**Back to the main menu without restarting the game.** Hamweard is an
[Anvil](https://github.com/happy-dwarf-team/anvil) plugin for Europa Universalis IV 1.37.5
(Steam, and GOG 1.37.5.2 and 1.37.5.0, Windows x64). **Back** on the country selection screen and **Exit to
Menu** in a campaign take you to the main menu in a few seconds, instead of closing and
starting the game again.

**Install guide:
[anvil.happydwarf.dev/plugins/hamweard](https://anvil.happydwarf.dev/plugins/hamweard/).**

It is a port of [EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch) by VulonLok,
under the MIT License (see [Thanks](#thanks)).

**Status:** tested in the lab in single player, in ironman and with mods, Anbennar among
them, up to 40 returns in one process (see [What was tested](#what-was-tested)). Multiplayer
had one test with a second player.

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
your latest save, as after a restart. Without the plugin both buttons restart the game as usual.

## Limitations

- **Gameplay.** `affects_gameplay = false`: a return does not change the simulation. The next
  campaign starts in the same process instead of a fresh one, and in the lab a save made after
  three returns matched a save from a fresh process, apart from the campaign id and the random
  seed. Anbennar matched too, after a return and after a plain **Back** from the lobby. That is
  a comparison of the campaigns and mods tested, not a proof for every mod.
- **Canals.** A canal changes the map itself, and the map outlives the campaign. The game's
  reset takes the canal's links out, but leaves other movement costs and naval distances
  behind. So the plugin copies them before the first canal and puts them back on a return.
  If they cannot be put back, that return restarts the game, as it does without the plugin.
- **Random New World restarts.** After a Random New World map, from a new campaign or a loaded
  save, Exit to Menu and Back restart the game, as they do without the plugin. The game's own
  restore of the map misses part of it: in the lab the next campaign gave Tortuga other
  missions than a fresh start did.
- **The Single Player list keeps a game bug.** Click a save in the list and start a new
  campaign from the same screen, and all province triggered modifiers are gone. The game
  does that too, with or without Hamweard. **Back** first clears it: the new campaign then
  matches one from a fresh start.
- **Multiplayer: no return, and one test with a second player.** In any multiplayer session,
  Exit to Menu and Back restart the game, as without the plugin. The plugin checks that itself:
  it stays loaded, in a lobby and in a game with other players, and Anvil does not take its
  changes out. Tested on Steam with a host and a client, both with Hamweard: the game's usual
  out-of-sync check, which compares the random-number count and seed, reported nothing in about
  four months of game, a client who left and came back into the running campaign included. The
  test did not use `-hardoos`, which compares all 140 checksums of the game state.
- **Multiplayer after a campaign needs a restart.** The game marks a process that has played
  as possibly corrupted, and only a fresh process is meant to go on to multiplayer. After a
  return the Multiplayer button does nothing, and its tooltip says why. Restart the game once.
- **The Steam lobby clean-up has not run with a lobby open.** It is ported from the original,
  and in a multiplayer session the game restarts instead.
- **Ironman: on, with nothing to allow.** It works on Steam and GOG: the game saves when you
  leave, the save loads again, and a new ironman campaign after a return says achievements can
  be earned. One achievement was earned for real on Steam in a campaign after a return, so the
  proof is one case and the game's own state. An `allow_ironman` entry for Hamweard in `anvil.toml` no longer does
  anything. An ironman save does not update the game's Continue game record, so after a return
  Continue game points at your last normal save. Load the ironman save from the Single Player
  list.
- **Mods: tested, not all of them.** No game file changes, so the checksum stays the same.
  Anbennar with Proper 2K UI and a playset of ten interface and graphics mods ran without
  trouble. A mod that does something unusual when a campaign ends may still behave
  differently.
- **Not seen in a test:** some message popups (an heir to name, a new colonial nation).
- **Memory still grows with each return, more with a big mod.** The game's reset does not
  free all it read. Hamweard frees what the lab found in the history and the country files
  (see [Memory a return used to lose](#memory-a-return-used-to-lose)): with Anbennar that was
  about 60 MB per **Back** and per campaign. What stays is mostly the last campaign's world and
  GUI: in the lab a **Back** left about 5 MB with Anbennar and 8 MB without mods, and in a row
  of returns the game grew by about 45 MB per return with Anbennar and 20 MB without mods. The time of a return
  stayed the same over 40 returns in a row. About eight windows per campaign stay in the GUI root too (see
  [Windows a campaign leaves behind](#windows-a-campaign-leaves-behind)), but they are not the
  cause. When less than 1 GB of the system's commit limit is left, a return restarts the game
  instead.
- EU4 1.37.5 on Steam and the GOG builds 1.37.5.2 and 1.37.5.0 (1.37.5.0 from Anvil 1.15). On any
  other build Anvil does not load the plugin. The GOG builds have no Steam lobby, so there is nothing to clean up there.

## Troubleshooting

Every return writes to `<EU4>/anvil/anvil.log`, in lines that start with `[hamweard]`. The
log says when the plugin chose a restart, and why.

| What you see | Why | What to do |
| --- | --- | --- |
| Exit to Menu or Back restarts the game | The plugin is not loaded | Open the Anvil panel (`Ctrl+Shift+F12`): is Hamweard loaded and approved? |
| The same, and the log says `the map could not be put back` | A canal changed the map and the saved copy no longer fits it | Nothing to fix: a campaign does not start on the changed map, so the plugin restarts the game. |
| The same, and the log says `multiplayer` | A multiplayer session ends with a restart | Expected. |
| The same, and the log says `Random New World` | Such a map changes more than a return can put back | Expected. |
| The same, and the log says `too little memory` | Less than 1 GB of the system's commit limit was left | Expected: the restart gives the memory back. |
| Anvil refuses to load Hamweard | The game is another build, or the original EU4 Menu Patch changed the same code | Use EU4 1.37.5 on Steam or GOG 1.37.5.2 or 1.37.5.0, and remove the Menu Patch DLL. |
| The Multiplayer button does nothing after a return | The process has played, and only a fresh one goes on to multiplayer | Restart the game once. |
| Continue game does not show your ironman campaign | The game does not record ironman saves there | Load it from the Single Player list. |
| Province triggered modifiers are gone in a new campaign | A game bug: a save was clicked in the list before the new campaign | Press **Back** after clicking a save, then start the campaign. |

A crash or anything odd after a return: open an
[issue](https://github.com/happy-dwarf-team/hamweard/issues) with `anvil.log`, and
`Documents/Paradox Interactive/Europa Universalis IV/logs/error.log`, from that sitting.
A question, or not sure it is a bug? Ask in `#help-and-bugs` on the team's
[Discord server](https://discord.happydwarf.dev/) with the Hamweard tag; what the team confirms
there is tracked in an issue.

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
  then Back. No hosting. From 1.2.0 the button does nothing after a return.
- **Ironman with the opt-in of that version:** Steam and GOG, new campaigns after a return,
  two in a row, the save on leaving and loading it again.
- **Mods:** Anbennar with Proper 2K UI Enhanced (9 returns, returns in 5.5 to 6.6 seconds),
  and a copy of a ten-mod playset (8 returns, one campaign over a year long).
- **Bellows 0.19.0 and the Anvil overlay alongside:** 23 returns in a row, the menu at
  163 frames per second at the end.
- **Bellows 1.3.0 with default settings and the overlay alongside (7 November 2026, Steam,
  Hamweard 1.2.0 and Anvil 1.13.0 from the release files):** after 5 returns without mods and
  3 with Anbennar, a new campaign matched one from a fresh process with Bellows, 1,681,409 and
  5,764,559 lines, only the campaign id differs. One run each, no crash.
- **GOG:** a save made on GOG, Continue game, ironman, a campaign of 400 days, 5 returns.

The state reset fixes of 1.1.2 were tested the same day. The same seed gave the same new
campaign byte by byte, the campaign
id apart, in a fresh process and after returns from campaigns started in 1444, 1453 and
1508, from a save of a 1508 start, from a 1519 save, and after Back with a later date or a
save picked in the list; on GOG after three returns as well. A ten-mod playset gave the same
result. The menu's checksum stayed the same.

Canals were tested on 5 November 2026: after a 1719 save with canals on Steam and on GOG, a
new campaign matched a fresh one, with no restart, 0 naval distances different and the
campaign id the only other difference. Anbennar, which applies a canal at every start,
matched a fresh start the same day, and returned from ironman without a restart.

Ironman on by default was tested on 5 November 2026, with no `anvil.toml` entry for the
plugin, on Steam and GOG: no warning, achievements can be earned, and a return without a
restart. On GOG a month of play after a return ran the game's achievement check on all 175
achievements, as in a fresh process, and the game state in the save on leaving was the same,
byte by byte, as with the game's own exit.

A hard return test ran on 6 November 2026, on 1.1.3 and the fixes after it, with the same
seed each time. Save lines are compared with a campaign from a fresh process.

| Run | Returns in one process | Save after them |
| --- | --- | --- |
| Steam, no mods | 40 (30 **Back**, 10 **Exit to Menu**) | 1,681,409 lines, only the campaign id differs |
| Steam, Anbennar | 7 | 5,764,559 lines, the same |
| Steam, Anbennar in ironman | 3 | the same |
| Steam, Warhammer Fantasy Universalis | 4 | 1,964,700 lines, the same |
| Steam, end of the game in 1821 from a 64.5 MB save | 1 | 1,681,452 lines, the same |
| Steam, the window of a lost game, then Quit | 1 | 1,681,409 lines, the same |
| GOG, no mods | 16, no crash, then 1 before a compared save | 1,648,632 lines, the same |

A return took 3.3 to 3.8 seconds with **Back** and 5.5 to 6 with **Exit to Menu** without mods,
and 5.8 to 6.3 and 8 to 10.7 with Anbennar. In that test two bugs came up and are fixed: a
Random New World map changed the next campaign's missions (such a return now restarts), and a
custom nation designed in the lobby and left behind with **Back** joined the next campaign.

The achievement list and the multiplayer check of 1.1.2 were tested that day too, with 27 returns in one process, 13 Exit to Menu and 14 Back, no restart and
no crash. After a return the list stayed empty, and a normal save had no `achievement`
block, also after an ironman save, a country change and a save that still has the block. An
ironman save loaded after each of those had its 52 achievements, and a new ironman
campaign said achievements can be earned.

The memory fixes were tested on 9 November 2026 (lab `docs/hamweard-memory-fix.md`), with
a heap tracer and in returns in a row with new and loaded campaigns, the save of a new
campaign after the returns compared with one from a fresh start (`-seed=12345`):

| Game | Returns in one process | Crashes | New campaign after the returns |
| --- | --- | --- | --- |
| Steam, Anbennar | 22 (12 campaigns, 5 of them loaded; before the fix for loaded saves) and 11 (8, 5 loaded) | 0 | 5,764,559 lines, the same as a fresh start with Hamweard and one without it |
| GOG 1.37.5.2, no mods | 22 (12, 5 loaded; before the fix for loaded saves) and 11 (8, 5 loaded) | 0 | 1,648,632 lines, the same |

Not tested: the Steam lobby clean-up with a lobby open, and the windows and modes
listed under [Limitations](#limitations). The multiplayer check was read from the game's
code first, then tested with a second player on 6 November 2026.

## What it changes in the game

| Site | The game | The plugin |
| --- | --- | --- |
| `0x815CAE`, the Exit to Menu branch of `CInGameIdler::Idle` | sets the exit and restart flags: the game quits and starts again | in single player releases the campaign's leftovers, queues a new main menu and resumes at the stock epilogue; in a multiplayer session, the game's restart |
| `0x111DFD7`, Back on the country selection screen | the same flags | in single player queues the switch for the menu's next tick; in a multiplayer lobby, the game's restart |
| slot `+0x20` of the frontend idler vtable | the frontend tick | runs the queued switch, destroys the last campaign's windows, resets the world once the old idler is gone, forgets the achievement list the last loaded save carried and puts a canal's map back, or restarts the game if it cannot, then the stock tick |
| slot `+0x30` of the frontend idler vtable | builds the menu, starting with the loading screen | clears the last campaign's map objects first, so unit banners do not stand on the loading screen |
| `0x468430`, `CustomCountryManager::Clear` | forgets the nations of the lobby, only when a campaign starts | calls it on every return too, or a nation designed and left behind by **Back** would join the next campaign |
| `0x890D00`, `CMapRandomize::Randomize` | builds a Random New World map | the same, and the plugin notes it: from then on a return takes the game's restart |
| `0x78F250`, `ApplyCanalLogic` | rewrites the map's neighbours for a canal | the same, and before the first canal the plugin copies the map's links and naval distances, to put them back after the world reset |
| `0x7E8470`, `CHistoryDataBase::Execute` | walks the history between two dates | does nothing while a return waits for its world reset: the new menu would walk it on the old world, which crashed after a start from a later date. In the reset's own walk it first validates the reloaded history, as the game does at startup: without that, triggers in history files stay unbound, and Anbennar's next campaign differed |
| `0x10F0510`, the main menu's Multiplayer button | opens the multiplayer screen, whatever the game's check says | does nothing after a return in a process that has played |
| `0x10EB5C0`, `CFrontEndView::GetMultiplayerAvailability` | gives the restart reason only with a Paradox session | gives it after a return without one too, so the button's tooltip always says why |
| `0x14E9030` (GUI manager create), `0x10A0735` (popup window), `0x803270`, `0x81AF10` | create windows, build the campaign GUI | the same, and the plugin notes which windows belong to the campaign |
| `0x5ED370`, `CIfEffect`'s deleting destructor | frees an `if` effect but not the `else_if`/`else` branches chained to it | while the world resets, a new campaign starts or a save loads, deletes those branches too; elsewhere the stock destructor alone (see [Memory a return used to lose](#memory-a-return-used-to-lose)) |
| `0x7E9490`, `CHistoryDataBase::MoveIntoGameState`; `0x7E9820`, `DestroyDatabase`; `0x5CF740`, `CGameStateLoadTask` | a new campaign deletes the history's effects; a save's load and the reset destroy the history | the same, with the branches freed for their duration; the game's exit stays the stock one |
| `0x233D8D0`, `CCountryDataBase` | each country keeps a table of its idea groups, which the country's destructor never frees | frees the tables with the game's own `delete[]` just before the world reset |
| `0x93AE10`, `CCountryModifiers`' destructor | rebuilds a country's modifiers in place and frees the list of its subjects' modifiers, but not the modifiers in it | deletes those modifiers first, in the whole game (see [Memory a long campaign lost](#memory-a-long-campaign-lost)) |

The world's `PotentialCorruption` flag stays as the game keeps it through its reset. Only
the `-nudge` reason (2), which the reset flattens to 1, is put back. Earlier versions, like
the original, cleared the flag after every return.

## Why a return takes a few seconds

A return took 3.5 to 4 seconds in the lab, and about 3 of those are the game's own world
reset (`ResetGame`). Almost all of that re-reads the databases and the history from disk,
which is what gives the next campaign a clean start, so the plugin does not skip it.
Loading a save from the menu is the game's own path, and Hamweard adds nothing to it.

## Memory a return used to lose

The game's reset destroys the history and the countries it read and reads them again, and it
loses part of what it destroys. 1.37.5 restarts the process instead, so it never adds up
there. The lab found two such leaks with a heap tracer (eu4-engine-lab, 9 November 2026):

- **`else_if`/`else` branches.** `CIfEffect`'s destructor frees the `if` but not `_pElse`
  (`+0xF0`), the chain of branches that follows it in a script, and nothing else points to
  them. Each time the history was destroyed, every such branch was lost with all it holds.
  Anbennar's country history calls scripted effects full of `else_if`: about 60 MB each
  time, at every **Back**, every new campaign and every save loaded. Without mods it is a few
  kilobytes. The plugin deletes the branches only while the world resets, a new campaign
  moves the history in, or a save's load destroys the old history; the rest of the game,
  its exit among it, keeps the stock destructor.
- **Idea group tables.** Each country gets a `bool[]` of its idea groups when the country
  files are read, and the country's destructor never frees it: 2.6 MB per return with
  Anbennar, 0.8 MB without mods. The plugin frees them with the game's own `delete[]` just
  before the reset.

## Memory a long campaign lost

This one grows while you play, with or without a return. Each time the game recalculates the
modifiers of a country with subjects (every day for some countries, and after many actions), it
gives the overlord one new modifier per subject (`CalcModifiersFromSubjects`) and keeps it in a
list (`ModifiersFromSubjectToOverlord`). The next recalculation throws the list away but not the
modifiers in it. In the lab that lost about 2 MB per year of play without mods (a heap tracer,
10 November 2026), so a full campaign from 1444 kept several hundred MB it never used again. The
plugin deletes those modifiers when the game rebuilds or destroys the list. Nothing else in the
game points to them but the modifier they were added to, which goes with the same list.

With both, the tracer found nothing left of either after a **Back**, a new campaign, or a
loaded one. What a return still leaves is mostly the last campaign's world and GUI (about
35 to 55 MB without mods and 70 to 100 MB with Anbennar after a campaign, as before), and a
small part of the history of each campaign's countries.

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
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target hamweard-check
```

The check target runs `anvil check --exe` on all 47 symbols and rewrites `symbols.lock`;
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
