# Glossary

Terms the code, the issues and the pull requests use for what a return to the main menu
leaves behind (decided in #106).

- **Carry-over**: state of a previous campaign that a new campaign can *read*, so that it
  could differ from a fresh start. The main goal of the return-state work is zero carry-over.
- **Leak**: memory of a previous campaign that nothing reaches any more. Fewer leaks is the
  second goal. A leak that something can still read (for example a cache keyed by address) is
  a carry-over.
- **Fresh start**: the same campaign started in a new game process, the reference a campaign
  after returns is compared with.
- **In scope** for carry-over: a new campaign after a return, and a save loaded from the main
  menu after a return. A save loaded from inside a campaign is the game's own path, which
  Hamweard takes no part in; only leaks are counted there.
