# Contributing

Every change goes to `main` through a pull request, which needs the `windows` (build)
and `conventional-title` checks to pass and is **squash-merged**: its title becomes the
commit on `main` and the changelog entry.

Titles follow [Conventional Commits](https://www.conventionalcommits.org/):
`<type>(<optional scope>): <summary in the imperative, lowercase, no period>`.

| Type | Use for | Release |
| --- | --- | --- |
| `feat` | something new for players | minor |
| `fix` | a bug fix | patch |
| `perf` | faster or lighter, same behaviour | patch |
| `revert` | reverting an earlier change | patch |
| `build` | CMake, the SDK version, packaging | patch |
| `docs`, `refactor`, `test`, `ci`, `chore` | everything else | none |

Before 1.0.0 a breaking change (`feat!:` with a `BREAKING CHANGE:` paragraph) bumps the
minor version.

**Game code and names.** This repository holds no code of eu4.exe and no
names from its debug symbols. `symbols.toml` verifies each address with
`sha256` and `size`, not with the game's bytes; write `bytes` while you work
and run Anvil's `tools/symbols-to-digest.py` before you commit. Name the
game's functions, classes and fields by their address and what they do,
unless eu4.exe itself shows the name (its RTTI or its own log and assert
texts). Our own symbol keys, addresses, offsets and signatures are fine.

When you change `symbols.toml`, run the `-check` target with `EU4_EXE` set to the
`eu4.exe` of each build in `builds` (Steam 1.37.5, GOG 1.37.5.2 and 1.37.5.0) and commit
the new `symbols.lock` in the same pull request; CI fails on a stale lock.

## The reset oracle

Hamweard's promise is that a campaign after a return is the campaign a fresh start would
give. The reset oracle checks it. Run it before every release that changes the reset:
`reset_world`, anything Hamweard keeps at the first menu and restores after `reset_game`,
the teardown, or the symbols they use. The Random New World and canal arms are needed for a
change to the map copy; their fresh side is the vanilla arm's. A pull request that fixes a carry-over puts the
arms it touches in its evidence. The lab (`eu4-engine-lab`, private) runs the oracle with
`scripts/hamweard-oracle.ps1`: one file per arm of the table below, and a comparison that applies the rules of
this section and prints each arm's verdict.

Each arm is a pair of runs of the same game with the same plugins, and the same start in both:

- **fresh**: a new process, the start, then the measured stretch;
- **after returns**: a new process, three campaigns each left with **Exit to Menu** after a
  few days, then the same start and the same stretch.

| Arm | Game | Start | The three campaigns before it |
| --- | --- | --- | --- |
| vanilla 1444 | Steam, no mods | a new campaign, first bookmark, first country | a new campaign, a loaded save, a new campaign |
| Russia 1719 | Steam, no mods | the Russia save of 17 March 1719, loaded from the menu | the basic tutorial; the same save with console commands (`infinite_combat`, `combat_dice 3`); a new campaign |
| late game | Steam, no mods | a save of 1 January 1821, loaded from the menu | the same save, a new campaign, the Russia save |
| Anbennar | Steam, Anbennar | a new campaign | three new campaigns |
| GOG | GOG 1.37.5.2, no mods | a new campaign | three new campaigns |
| Random New World | Steam, no mods | a new campaign, as vanilla 1444 | a Random New World campaign, a new campaign, a Random New World campaign |
| canals | Steam, no mods | a new campaign, as vanilla 1444 | a new campaign with the Kiel canal built from the console (`great_project kiel_canal`), the Russia save, the same canal campaign |

The measured stretch:

- the game starts with `-seed=12345 -randomlog`;
- the console's `observe` right after the start;
- 60 days at speed 5, in stretches of 30 days, every event taking its first option;
- a plain-text save at the end.

The AI breaks a few ties by an object's address, which differs between processes. Both runs of an
arm also load a plugin that breaks those ties by the object's id instead (the lab's tie-break
plugin), so that the heap layout does not decide them.

An arm passes when, for every day of the stretch, every line of the random log (without
frame numbers and command ids, and without `client_ping` commands, which follow the wall
clock) is the same in both runs, and the two saves differ in no line but `campaign_id` and
`campaign_length` (the time played, which follows the wall clock, not the game).

One difference does not come from the game state. The AI posts its commands from its own
thread, and the game executes a command in the frame it arrives, so on a busy day one run
can execute an AI command a day later than the other. Two fresh starts differ that way now
and then. When an arm's first difference is the same command executed a day later, and
every day before it is the same, run the fresh side again: the arm passes when the run
after the returns equals one of the fresh runs.

The other comes from reading the log. The random log is a ring in the game's memory, and a
reader outside the game can fall behind on a frame that writes hundreds of thousands of lines
(a country recruiting a whole army at once): it then misses or repeats a stretch. Such a day
shows command ids that one log repeats or lacks. It does not fail the arm when the executed
command ids are the same set apart from those, the draw count at the end is equal and the saves
pass; report the day as lost to the capture.

Report each arm with the game's build, the Hamweard build, the draw count at the end and
the result; for an arm that fails, the first differing day and line.
