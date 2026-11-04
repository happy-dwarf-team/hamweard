**Leave a campaign and you are back in the main menu in seconds, without the game restarting. The dwarf walks home; the forge stays lit.**

Normally **Exit to Menu** in a campaign and **Back** on the country selection screen close EU4 and start it again: the logos, the loading screen, the whole wait. With Hamweard both take you straight to the main menu, usually in about 4 seconds, and **Continue game** points at your latest save.

## What you get
- **No restart** after Exit to Menu or Back: the main menu comes back in the same game.
- **Fewer crashes on the way home:** Hamweard also clears the windows each campaign leaves behind, which in our tests made a later campaign crash.
- **Tested in single player:** dozens of returns with wars, naval battles, ironman and big mods, and no crash.
- **Honest limits:** your next campaign starts in a game that already played one. We compared saves and almost nothing carried over, but Anvil treats Hamweard as a mod that affects gameplay: it is off in multiplayer and off in ironman unless you allow it.

Hamweard is a port of EU4 Menu Patch by VulonLok, under the MIT License. The original needs a separate loader; Hamweard runs on Anvil.

### Why "Hamweard"?
Old English *hāmweard* means "homeward". The mod takes you from a campaign back to the main menu as if back home, without a restart on the way. Our other tools are named after the forge (Anvil, Bellows, Tongs, Etch); this one is the walk home after a day at it.

![](https://i.imgur.com/Uf8Z21n.png)

## Install
Hamweard runs on EU4 1.37.5 (Steam, Windows; the GOG build 1.37.5.2 too, see the FAQ). On any other version it stays out of the way and the buttons restart the game as usual. If you use the original EU4 Menu Patch, remove it first: both change the same part of the game.

1. **Install [Anvil](https://anvil.happydwarf.dev/) 1.9 or newer.** Download it, run it and click **Install Anvil**. If Windows warns, choose **More info**, then **Run anyway**. You do this once; the Workshop can't do it for you.
2. **Subscribe** and enable Hamweard in your playset.
3. **Start the game.** Anvil says a plugin is waiting: press **Ctrl+Shift+F12**, open **Mods** and approve Hamweard.
4. **Restart the game.** From then on, **Exit to Menu** and **Back** take you home without one.

Anvil asks again after an update; the version you approved keeps working until then.

![](https://i.imgur.com/Uf8Z21n.png)

## What it doesn't touch
- No game files: the files the game checks stay the same, so ironman achievements stay available.
- **Gameplay, honestly:** your next campaign starts in a game that already played one, instead of a fresh one. A save made after a few returns matched one from a fresh start; the only thing we saw carry over is an achievements list after an ironman campaign, which changed nothing in our tests. We can't promise nothing else does, so Anvil treats Hamweard as a mod that affects gameplay.
- **Ironman:** off unless you allow it in Anvil. Allowed, it worked in our tests, saves included.
- **Multiplayer:** always off. In a multiplayer game Anvil keeps Hamweard out, and Exit to Menu restarts the game as usual.
- **Canals:** a canal changes the map itself, so once a save with a canal has loaded, Exit to Menu and Back restart the game as usual until you close it.
- **Multiplayer after a campaign:** the game itself asks for a restart first, and Hamweard leaves that check alone.
- Unsubscribe and it's gone. Nothing is left in your saves.

![](https://i.imgur.com/Uf8Z21n.png)

## Current state
- Tested on our machine in single player: Back, Exit to Menu, Continue game, new games and saving after a return, changing country, battles at land and sea, ironman, the multiplayer menu, Anbennar, a ten-mod playset and Bellows. 66 returns in one day, no crash.
- On the GOG build: a GOG save, Continue game, ironman and a year-long campaign. No crash.
- Not tested yet: a multiplayer game itself. Tried something new? Tell us on [GitHub](https://github.com/happy-dwarf-team/hamweard/issues).
- Each return leaves a few small windows behind (about 15 MB). After many returns, a full restart now and then does no harm.

![](https://i.imgur.com/Uf8Z21n.png)

## More from Happy Dwarf Team
- **[Anvil](https://anvil.happydwarf.dev/)**: the free loader that lets plugins like Hamweard into EU4, with a Manager to approve them and keep them updated.
- **[Bellows](https://steamcommunity.com/sharedfiles/filedetails/?id=3823747170)**: and when you do start the game, it reaches the main menu in about a third of the time.
- **[Mei](https://steamcommunity.com/sharedfiles/filedetails/?id=3832685486)**: Chinese, Japanese and Korean text in EU4, for the translation mod of your choice.
- **[Etch](https://etch.happydwarf.dev/)**: for interface modders, see your `.gui` window as the game draws it, right in VS Code.
- Everything we make: [happydwarf.dev](https://happydwarf.dev/)

![](https://i.imgur.com/Uf8Z21n.png)

### Alternative download
[GitHub](https://github.com/happy-dwarf-team/hamweard), also for the GOG build. The research behind the port: [Anvil issue #105](https://github.com/happy-dwarf-team/anvil/issues/105). The plugin page: [anvil.happydwarf.dev/plugins/hamweard](https://anvil.happydwarf.dev/plugins/hamweard/).

![](https://i.imgur.com/Uf8Z21n.png)

## FAQ

### Why do I have to approve it?
Hamweard is a real program running inside your game, not just a pile of text files. Anvil makes sure nothing like that runs without your nod. Trust, but verify. Dwarf law.

### Exit to Menu still restarts the game.
Check the **Mods** tab in Anvil's panel (Ctrl+Shift+F12): it says whether Hamweard is approved, and if it was kept out, why (multiplayer, an ironman game you have not allowed it in, another game version). A dwarf who isn't let in can't show you the way home.

### Does it work with Bellows?
Yes. They do different jobs, Bellows the start of the game and Hamweard the way back to the menu, and they don't change the same parts of the game. We ran them side by side for 23 returns in a row without a hitch.

### Does it work in ironman or with big mods?
Ironman, yes, if you allow it in Anvil (achievements stay on, but a plugin that changes the game may break Paradox's rules for them). Continue game won't show your ironman campaign: load it from Single Player. Mods: Anbennar and a ten-mod playset ran clean, but no dwarf has walked every road.

### Is it the same as EU4 Menu Patch?
It is a port of it, with the original author's research and licence. It runs on Anvil instead of its own loader, and it also clears the windows a campaign leaves behind, which in our tests made a later campaign crash. Use one or the other, not both.

### I play on GOG. Can I use it?
Yes, on the GOG build 1.37.5.2. GOG has no Workshop, so install Hamweard from the **Catalog** tab in Anvil Manager, or from the release zip on [GitHub](https://github.com/happy-dwarf-team/hamweard/releases). The road home is the same from either gate.

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
