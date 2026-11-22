# Changelog

## [1.7.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.6.0...v1.7.0) (2026-11-22)


### Features

* return to the menu after a Random New World map ([#119](https://github.com/happy-dwarf-team/hamweard/issues/119)) ([2c2a0e8](https://github.com/happy-dwarf-team/hamweard/commit/2c2a0e89e61a492ea306f8504b73b369dfe76f5e))


### Bug Fixes

* clear the AI's strength factor caches at a return ([#108](https://github.com/happy-dwarf-team/hamweard/issues/108)) ([6a64b7d](https://github.com/happy-dwarf-team/hamweard/commit/6a64b7de4f675e3f82d6824a31fb6727812307eb))
* free every leader the old campaign left at a return ([#112](https://github.com/happy-dwarf-team/hamweard/issues/112)) ([80c3340](https://github.com/happy-dwarf-team/hamweard/commit/80c33400e66a8f1a4b94355ac812beeabe6ba8eb))
* free the estates' temporary modifiers at a return ([#113](https://github.com/happy-dwarf-team/hamweard/issues/113)) ([cc9ac28](https://github.com/happy-dwarf-team/hamweard/commit/cc9ac28fc7d4bd7e3b731cd0a703e3200841a36b))
* free the leaders of a save's characters at a return ([#111](https://github.com/happy-dwarf-team/hamweard/issues/111)) ([918ebb2](https://github.com/happy-dwarf-team/hamweard/commit/918ebb27e8919dc92fb52eaf568221d58aa1818d))
* free the old campaign's leaders once per reset ([#116](https://github.com/happy-dwarf-team/hamweard/issues/116)) ([013e317](https://github.com/happy-dwarf-team/hamweard/commit/013e3173322a9b4e8e5ba1a1dd69188479a6781c))
* free the old campaign's rebel factions at a return ([#117](https://github.com/happy-dwarf-team/hamweard/issues/117)) ([b4cb2bd](https://github.com/happy-dwarf-team/hamweard/commit/b4cb2bd1e13b1e7275f765e115f8d2ca6ef0d31c)), closes [#107](https://github.com/happy-dwarf-team/hamweard/issues/107)
* free what the old world loses at a return and at a save's load ([#121](https://github.com/happy-dwarf-team/hamweard/issues/121)) ([679f84f](https://github.com/happy-dwarf-team/hamweard/commit/679f84fc32a5509e33618d0e4c2cd2f91ee8c6c8))
* say when the strength factor caches could not be cleared ([#115](https://github.com/happy-dwarf-team/hamweard/issues/115)) ([0b5c572](https://github.com/happy-dwarf-team/hamweard/commit/0b5c572da508c73ce5f59b1c3fcaebd40f605415))

## [1.6.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.5.0...v1.6.0) (2026-11-19)


### Features

* log the game's memory after each return ([#91](https://github.com/happy-dwarf-team/hamweard/issues/91)) ([b2efcea](https://github.com/happy-dwarf-team/hamweard/commit/b2efcea0adff292918255eb9dd36663b3b7decf9))


### Bug Fixes

* free the AI's army agents at a return ([#105](https://github.com/happy-dwarf-team/hamweard/issues/105)) ([3e0ea4b](https://github.com/happy-dwarf-team/hamweard/commit/3e0ea4b2c54ae7a0e003e88a4a357ca9fe81e792))
* free the battles of a war's history when the world is reset or a save loads ([#101](https://github.com/happy-dwarf-team/hamweard/issues/101)) ([2e4ca45](https://github.com/happy-dwarf-team/hamweard/commit/2e4ca45be5e83819b9283593dd0383df3e65a64c))
* free the historical characters of a save's country history at a return ([#103](https://github.com/happy-dwarf-team/hamweard/issues/103)) ([cd38c7f](https://github.com/happy-dwarf-team/hamweard/commit/cd38c7f9a33ea48b21ee51f24dc0a8e8d389d0c0))
* free the windows and start dialogs that their owners leave at a return ([#102](https://github.com/happy-dwarf-team/hamweard/issues/102)) ([b648b9a](https://github.com/happy-dwarf-team/hamweard/commit/b648b9a6d9920d0f66301a60cacee50d998feb3e))
* let the Paradox launcher see the Workshop item as made for 1.37 ([#99](https://github.com/happy-dwarf-team/hamweard/issues/99)) ([8c865ce](https://github.com/happy-dwarf-team/hamweard/commit/8c865ce38e5c7a6ab2a40e30b7d211a03a2e8f13))
* name provinces after their owners again after a save is clicked ([#92](https://github.com/happy-dwarf-team/hamweard/issues/92)) ([fe1f233](https://github.com/happy-dwarf-team/hamweard/commit/fe1f2338f8881ac89ba6a5b216e122b285831712))
* put back the console's and the tutorials' switches on each return ([#96](https://github.com/happy-dwarf-team/hamweard/issues/96)) ([fbfe1a6](https://github.com/happy-dwarf-team/hamweard/commit/fbfe1a6b42ed3db55f30c8e92c9a7d9a09f64d3e))
* validate the history the setup screen reloads after a save is clicked ([#88](https://github.com/happy-dwarf-team/hamweard/issues/88)) ([e80c6ad](https://github.com/happy-dwarf-team/hamweard/commit/e80c6ade0fd7e931178397fc5240d2467a0a2351))

## [1.5.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.4.0...v1.5.0) (2026-11-15)


### Features

* Hold Shift when you click Back on the country selection, or Quit after Exit to Menu, and the game restarts as it would without Hamweard, with the same mods and launch options. Use it before a multiplayer game, which needs a fresh start. ([#82](https://github.com/happy-dwarf-team/hamweard/issues/82)) ([66e7975](https://github.com/happy-dwarf-team/hamweard/commit/66e7975568cf1aecd07cc1bf3885ebc0866ae963))

## [1.4.0](https://github.com/happy-dwarf-team/hamweard/compare/v1.3.4...v1.4.0) (2026-11-12)

### Features

* With Anvil 1.20 or newer, Hamweard's description shows in eight languages; the texts are drafts that native speakers have not checked yet. ([#77](https://github.com/happy-dwarf-team/hamweard/issues/77)) ([2a072ce](https://github.com/happy-dwarf-team/hamweard/commit/2a072ced3fe4f5e53e4630e7dad7e0be91dc097c))

### Bug Fixes

* A return to the main menu no longer leaves behind the touch-gesture objects of the game's text boxes, about 0.6–0.8 MB per campaign. ([#79](https://github.com/happy-dwarf-team/hamweard/issues/79)) ([3aa53fb](https://github.com/happy-dwarf-team/hamweard/commit/3aa53fbbb880dedb8c37fc22dc327d6ecbb32d0b))

## [1.3.4](https://github.com/happy-dwarf-team/hamweard/compare/v1.3.3...v1.3.4) (2026-11-11)

### Bug Fixes

* A return to the main menu and a save's load now free the delayed events the old campaign had already fired. When a `country_event` with `days` fires, the game takes it off the country's waiting list but never frees it, and nothing points to it any more. Hamweard notes these events as they fire and frees them once their campaign is gone: after a return and after a load. While you play, the game runs as before, so within one campaign they still pile up (about 0.17 MB a year in Anbennar after the first month). Anbennar's start-up events wait a few days, so the lab's heap trace found about 2.4 MB of them in each 5-day Anbennar campaign, and about none without mods. In the lab with Anbennar on Steam and the same seed, a new campaign after 2 returns drew the same random numbers in the same order as the game without Hamweard over 10 days, and its save differed only in the campaign id. 11 returns in one process, with 8 campaigns of which 5 were loaded, ran without a crash in Anbennar on Steam and in the base game on GOG 1.37.5.2. Hamweard freed 6,033 events after each new 3-day Anbennar campaign and 10,061 after each loaded one, and none in the base game. The saving is too small to show in the game's total memory. On GOG 1.37.5.0 only the game addresses were checked, without playing ([#74](https://github.com/happy-dwarf-team/hamweard/issues/74)) ([455c041](https://github.com/happy-dwarf-team/hamweard/commit/455c04112686720dacb5521170ce5ef5d5a16aca))

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
