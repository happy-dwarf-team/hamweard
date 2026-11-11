# Changelog

## [1.3.3](https://github.com/happy-dwarf-team/hamweard/compare/v1.3.2...v1.3.3) (2026-11-11)

### Bug Fixes

* A new campaign after a return to the main menu now plays out like the same campaign in a freshly started game. The game keeps one shared stand-in for "no ruler", "no heir" and "no consort", used by every country that has none, and events could change it: in Anbennar an event writes a culture into the "no heir" of a country without an heir. That change outlived the return, so in the next campaign an event's condition came out differently, and after 10 days its rebel factions, missions and battles differed from the same campaign started in a fresh game. Hamweard now puts the culture, religion, dynasty and flags of these three stand-ins back to what they were at the first main menu, on each return. In the lab with Anbennar on Steam and the same seed, a campaign after 2 returns drew the same random numbers in the same order as a fresh game over 10 days, in 2 of 2 runs, and its save matched. A base-game campaign on GOG 1.37.5.2 after 6 returns matched a fresh one after 30 days. On Steam without mods and on GOG 1.37.5.0 only the game addresses were checked, without playing. ([#72](https://github.com/happy-dwarf-team/hamweard/issues/72)) ([24dc8d2](https://github.com/happy-dwarf-team/hamweard/commit/24dc8d25d41a9298e682bb2e00292f9b51ad41aa))

## [1.3.2](https://github.com/happy-dwarf-team/hamweard/compare/v1.3.1...v1.3.2) (2026-11-10)

### Bug Fixes

* A long campaign no longer slowly fills memory with the modifiers a country gets from its subjects. Every time the game recalculates an overlord's modifiers, which it does every day for some countries, it makes a new modifier for each subject and forgets to free the old one. This was the main memory leak of normal play, with or without a return to the menu. Hamweard now frees those modifiers when the game drops them, all through the campaign. In the lab's heap trace of one year of the base game the game lost 0.16 MB instead of 2.23 MB, and none of it to these modifiers instead of 1.97 MB; without the fix Anbennar lost 4.8 MB a year to them. In the lab's tests the game did not crash, and `error.log` had the same lines as without Hamweard. Saves were compared line by line with saves made without Hamweard: a fresh start was the same at day 0 in the base game and in Anbennar, and after 10 days in Anbennar, and a new base-game campaign after 5 returns was the same after 30 days. Two saves differed later in the campaign. A new Anbennar campaign after 2 returns differed after 10 days, first in its rebel factions; a control run after this release gave the same save, byte for byte, with Hamweard 1.3.1, so the difference comes from returning to the menu and not from this fix. A fresh base-game start differed after 30 days, while the same build after 5 returns matched; this is thought to come from the test's timing and was not repeated. ([#68](https://github.com/happy-dwarf-team/hamweard/issues/68))

## [1.3.1](https://github.com/happy-dwarf-team/hamweard/compare/v1.3.0...v1.3.1) (2026-11-09)

### Bug Fixes

* Going back to the main menu no longer leaves the old campaign's scripts in memory. The game forgets to free the `else` branches of `if` blocks and some idea group tables each time it drops its history: at a return to the menu, at the start of a new campaign and when a save loads. Hamweard now frees them at those three moments, and only then; at the game's exit nothing changes. In the lab with Anbennar on Steam, memory left over after a return to the menu went from about 74 MB to about 4.5 MB, and after a campaign and its exit from about 70 MB to 2.4 MB. Over the whole test the game's memory grew by 424 MB instead of 937 MB. 22 returns to the menu in vanilla (new and loaded campaigns) had no crash, and saves made after the returns were the same as saves from a fresh start, in vanilla and in Anbennar. ([#61](https://github.com/happy-dwarf-team/hamweard/issues/61))

## [1.3.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.2.1...v1.3.0) (2026-11-09)

### Features

* Hamweard runs on the GOG build 1.37.5.0, the version GOG Galaxy installs when you roll EU4 back. On that build it loads with Anvil 1.15.0 or later, which knows the build. Tested in the lab on a copy of a GOG 1.37.5.0 install with Bellows, Tongs and Mei: Exit to Menu returned to the main menu in the same process, a new campaign started after it, and an autosave loaded. Nothing changes on Steam or on GOG 1.37.5.2. ([#60](https://github.com/happy-dwarf-team/hamweard/issues/60))

## [1.2.1](https://github.com/happy-dwarf-team/hamweard/compare/v1.2.0...v1.2.1) (2026-11-09)


### Bug Fixes

* explain the Multiplayer click after a return without a Paradox session ([#48](https://github.com/happy-dwarf-team/hamweard/issues/48)) ([78ca9ec](https://github.com/happy-dwarf-team/hamweard/commit/78ca9ec051447dcc950bd37bd51fc41136762157))

## [1.2.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.1.4...v1.2.0) (2026-11-06)


### Features

* run in ironman by default ([#38](https://github.com/happy-dwarf-team/hamweard/issues/38)) ([054477d](https://github.com/happy-dwarf-team/hamweard/commit/054477d27d8ed8e0e507ee30f457ec6e8a0cea7d))


### Bug Fixes

* keep a played process out of multiplayer after a return ([#43](https://github.com/happy-dwarf-team/hamweard/issues/43)) ([42296de](https://github.com/happy-dwarf-team/hamweard/commit/42296de176152b1275956451565ee6ad55a67650))

## [1.1.4](https://github.com/happy-dwarf-team/hamweard/compare/v1.1.3...v1.1.4) (2026-11-06)


### Bug Fixes

* forget a custom nation designed in the lobby ([#42](https://github.com/happy-dwarf-team/hamweard/issues/42)) ([d2c9915](https://github.com/happy-dwarf-team/hamweard/commit/d2c9915356741dc48ff14488897bc235f162b67d))
* restart after a Random New World map ([#41](https://github.com/happy-dwarf-team/hamweard/issues/41)) ([f43c82d](https://github.com/happy-dwarf-team/hamweard/commit/f43c82d7f554ff8b580ecc13d7a5593003216533))
* restart instead of a return when memory runs out ([#39](https://github.com/happy-dwarf-team/hamweard/issues/39)) ([e4e34a3](https://github.com/happy-dwarf-team/hamweard/commit/e4e34a35d082ad37e80cf11487ac53271d1e5031))

## [1.1.3](https://github.com/happy-dwarf-team/hamweard/compare/v1.1.2...v1.1.3) (2026-11-06)


### Bug Fixes

* match a fresh campaign after a return with canals and mods ([#32](https://github.com/happy-dwarf-team/hamweard/issues/32)) ([505b2ee](https://github.com/happy-dwarf-team/hamweard/commit/505b2ee4342647208c995018b2223b03b23def92))

## [1.1.2](https://github.com/happy-dwarf-team/hamweard/compare/v1.1.1...v1.1.2) (2026-11-04)


### Bug Fixes

* clear the achievement list and leave multiplayer sessions by restart ([#26](https://github.com/happy-dwarf-team/hamweard/issues/26)) ([b0f4e64](https://github.com/happy-dwarf-team/hamweard/commit/b0f4e64297868dedcec7b49e8671d35f4fcc152f))
* stop the crash after a later start and leave less behind on a return ([#23](https://github.com/happy-dwarf-team/hamweard/issues/23)) ([71a9886](https://github.com/happy-dwarf-team/hamweard/commit/71a9886ded3da7790a0a36b532d9966add20cc2a))

## [1.1.1](https://github.com/happy-dwarf-team/hamweard/compare/v1.1.0...v1.1.1) (2026-11-04)


### Bug Fixes

* ship the licence with the plugin ([#16](https://github.com/happy-dwarf-team/hamweard/issues/16)) ([fec5bac](https://github.com/happy-dwarf-team/hamweard/commit/fec5baccdca151b7f182857262144583f55ec411))

## [1.1.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.0.0...v1.1.0) (2026-11-03)


### Features

* run on the GOG build 1.37.5.2 ([#12](https://github.com/happy-dwarf-team/hamweard/issues/12)) ([c785510](https://github.com/happy-dwarf-team/hamweard/commit/c7855103a2f08673205aa759568e6bf7c5b3ee47))

## 1.0.0 (2026-11-02)


### Features

* back to the main menu without restarting the game: Exit to Menu in a campaign and Back on the country selection screen return to the main menu in the same process, a port of EU4 Menu Patch by VulonLok ([64e28c5](https://github.com/happy-dwarf-team/hamweard/commit/64e28c56ff000cd3acb916a1a218dad909c37e51)) ([#6](https://github.com/happy-dwarf-team/hamweard/issues/6)) ([549fdf3](https://github.com/happy-dwarf-team/hamweard/commit/549fdf3c8c2030d1974cd5b3d3797d2d17a0763e))
* destroy the windows and message popups each campaign leaves behind, so the next campaigns do not crash ([5d21efd](https://github.com/happy-dwarf-team/hamweard/commit/5d21efde604a971e6591de97648df884a602cdde)) ([6274ed7](https://github.com/happy-dwarf-team/hamweard/commit/6274ed7e19f9643c8d9d748af143011980346d03)) ([#2](https://github.com/happy-dwarf-team/hamweard/issues/2)) ([eb13b9a](https://github.com/happy-dwarf-team/hamweard/commit/eb13b9a40ffaef0f0d3a1d5f6100a62448780257))
