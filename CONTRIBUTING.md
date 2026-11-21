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

When you change `symbols.toml`, run the `-check` target with `EU4_EXE` set and commit
the new `symbols.lock` in the same pull request; CI fails on a stale lock.

## The reset oracle

Hamweard's promise is that a campaign after a return is the campaign a fresh start would
give. The reset oracle checks it. Run it before every release that changes the reset:
`reset_world`, anything Hamweard keeps at the first menu and restores after `ResetGame`,
the teardown, or the symbols they use. A pull request that fixes a carry-over puts the
arms it touches in its evidence.

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

The measured stretch:

- the game starts with `-seed=12345 -randomlog`;
- the console's `observe` right after the start;
- 60 days at speed 5, in stretches of 30 days, every event taking its first option;
- a plain-text save at the end.

The AI breaks a few ties by an object's address, which differs between processes. Both runs of an
arm also load a plugin that breaks those ties by the object's id instead (the lab's tie-break
plugin), so that two fresh starts give the same campaign.

An arm passes when, for every day of the stretch, the random log's draws (source line and
value) and its executed commands (without frame numbers and command ids) are the same in
both runs, and the two saves differ in no line but `campaign_id`. Report each arm with the
game's build, the Hamweard build, the draw count at the end and the result; for an arm that
fails, the first differing day and draw.
