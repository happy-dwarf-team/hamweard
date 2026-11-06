# Hamweard: repository rules

- Anvil plugin that returns to EU4's main menu without restarting the game. A new campaign after
  a return must match one from a fresh start; a change that can break that is tested against a
  fresh start, as the README's testing notes show.
- [CONTRIBUTING.md](CONTRIBUTING.md) lists the title types, the required checks and
  `symbols.toml` with its lock. Run the build and `ctest` before finishing a change; CI runs them.
- English in code and docs. Do not imply affiliation with Paradox Interactive.
- The agent owns the pull request flow: it works on a branch with Conventional Commits,
  pushes it and opens the PR as a draft. Once ready (green CI, the change tested as above,
  no blocking review findings, not put on hold by the user) it marks the PR ready and
  squash-merges it; GitHub does not allow auto-merge on a draft. Pull requests from other
  contributors get a review and a recommendation; the maintainer decides on merging them.
  Do not bypass CI or branch protection, and do not change repository settings beyond what
  auto-merge needs.
