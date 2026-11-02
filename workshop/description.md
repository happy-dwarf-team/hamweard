**Leave a campaign and you are back in the main menu in seconds, without the game closing and starting again. The dwarf walks home; the forge stays lit.**

![](https://i.imgur.com/Uf8Z21n.png)

## Overview
Normally **Exit to Menu** in a campaign, and **Back** on the country selection screen, close EU4 and start it again from scratch: the logos, the loading screen, the whole wait. With Hamweard both take you straight back to the main menu, in the same game, usually in about 4 seconds. **Continue game** then points at your latest save, as it would after a restart.

Hamweard is a port of EU4 Menu Patch by VulonLok, under the MIT License. The original needs a separate loader; Hamweard runs on Anvil, and it also cleans up the windows each campaign leaves behind, which in our tests made a later campaign crash.

### Why "Hamweard"?
Old English *hāmweard* means "homeward". The mod takes you from a campaign back to the main menu as if back home, without a restart on the way. Our other tools are named after the forge (Anvil, Bellows, Tongs, Etch); this one is the walk home after a day at it.

![](https://i.imgur.com/Uf8Z21n.png)

## Before you subscribe
- **[Anvil](https://anvil.happydwarf.dev/) 1.5 or newer** must be installed in the game folder. Get it from [anvil.happydwarf.dev](https://anvil.happydwarf.dev/) and press Download. The Workshop can't install it for you: a dwarf has to carry it in by hand, once.
- **EU4 1.37.5** (Steam, Windows). On any other version Hamweard stays out of the way and the buttons restart the game as usual.
- If you use the original EU4 Menu Patch, remove it first: both change the same part of the game.

![](https://i.imgur.com/Uf8Z21n.png)

## Getting it running
1. Subscribe and enable Hamweard in your playset.
2. Start the game. Anvil will tell you a new plugin is waiting: press **Ctrl+Shift+F12**, open **Mods** and approve Hamweard. You can also approve it before playing, in Anvil Manager.
3. Restart. From then on, **Exit to Menu** and **Back** take you home without one.

Anvil asks again after every update. Until you approve, the version you trusted before keeps working.

![](https://i.imgur.com/Uf8Z21n.png)

## What it doesn't touch
- No game files: the files the game checks stay the same, so ironman achievements stay available.
- **Gameplay, honestly:** your next campaign starts in a game that already played one, instead of a fresh one. We saw nothing carry over, but we can't promise nothing ever does, so Anvil treats Hamweard as a mod that affects gameplay.
- **Ironman:** off unless you allow it in Anvil. We have not tested it there yet.
- **Multiplayer:** always off. In a multiplayer game Anvil keeps Hamweard out, and Exit to Menu restarts the game as usual.
- Unsubscribe and it's gone. Nothing is left in your saves.

![](https://i.imgur.com/Uf8Z21n.png)

## Current state
- Tested on our machine in single player: Back, Exit to Menu, loading the same save again, Continue game, and twelve campaigns in a row in two sittings, some of them two or three years long with a war, a battle and a peace. No crash.
- Not tested yet: multiplayer, ironman, other mods alongside (big overhauls included) and naval battles. If you try one, tell us how it went on [GitHub](https://github.com/happy-dwarf-team/hamweard/issues).
- Each return leaves a few small windows behind. After many returns in one sitting, a full restart now and then does no harm.

![](https://i.imgur.com/Uf8Z21n.png)

## More from Happy Dwarf Team
- **[Anvil](https://anvil.happydwarf.dev/)**: the free loader that lets plugins like Hamweard into EU4, with a Manager to approve them and keep them updated.
- **[Bellows](https://steamcommunity.com/sharedfiles/filedetails/?id=3823747170)**: and when you do start the game, it reaches the main menu in about a third of the time.
- **[Etch](https://etch.happydwarf.dev/)**: for interface modders, see your `.gui` window as the game draws it, right in VS Code.
- Everything we make: [happydwarf.dev](https://happydwarf.dev/)

![](https://i.imgur.com/Uf8Z21n.png)

### Alternative download
[GitHub](https://github.com/happy-dwarf-team/hamweard). The research behind the port: [Anvil issue #105](https://github.com/happy-dwarf-team/anvil/issues/105).

![](https://i.imgur.com/Uf8Z21n.png)

## FAQ

### Why do I have to approve it?
Hamweard is a real program running inside your game, not just a pile of text files. Anvil makes sure nothing like that runs without your nod. Trust, but verify. Dwarf law.

### Exit to Menu still restarts the game.
Check the **Mods** tab in Anvil's panel (Ctrl+Shift+F12): it says whether Hamweard is approved, and if it was kept out, why (multiplayer, an ironman game you have not allowed it in, another game version). A dwarf who isn't let in can't show you the way home.

### Does it work with Bellows?
They do different jobs, Bellows the start of the game and Hamweard the way back to the menu, and they don't change the same parts of the game. We have not tested them side by side yet, so tell us if anything looks odd.

### Is it the same as EU4 Menu Patch?
It is a port of it, with the original author's research and licence. It runs on Anvil instead of its own loader, and it also clears the windows a campaign leaves behind, which in our tests made a later campaign crash. Use one or the other, not both.

### What is that little dwarf head on the thumbnail?
If you check the mod's GitHub page, you will notice the repository belongs to Happy Dwarf Team.

I like dwarves.
I like being happy.
I am, in fact, a happy dwarf myself.

![](https://i.imgur.com/Uf8Z21n.png)

## Credits
**VulonLok** ([EU4 Menu Patch](https://github.com/YozoraTempest/EU4-Menu-Patch), MIT) for working out how to get home without a restart, and for sharing it so others could build on it. Thank you.
**Anvil** for letting a dwarf into the forge.
**Everyone who sat through the logos one more time just to start a new campaign**: go home, it's quicker now.
