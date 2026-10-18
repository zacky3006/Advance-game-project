DIRTY WORK - WINDOWS DEVELOPMENT BUILD

Keep this entire folder together. ArenaGame.exe needs the other folders/files.

SOLO
Open ArenaGame.exe, then choose PLAY GAME.

TWO PLAYERS ON ONE PC (Lab 13 / loopback)
1. Open HOST.bat and wait for the MainMenu to load.
2. Open JOIN.bat. It joins the host at 127.0.0.1, port 7777.
3. Once both menus have loaded, choose PLAY GAME to start the team intro.
4. Both players enter DemoMap together. Space votes to skip the intro.
The launchers cap each game at 60 FPS to limit load when running two copies.

TWO PCs ON THE SAME LAN / WI-FI
1. Copy this entire game folder to both PCs.
2. The host opens HOST.bat and allows the game through Windows Firewall
   if prompted and permitted by the computer/network administrator.
3. On the host, run ipconfig and find the active adapter's IPv4 address.
4. On the second PC, open a Command Prompt in this folder and run:
   JOIN.bat 192.168.1.10
   Replace that example with the host's actual IPv4 address.
   Or edit JOIN_ADDRESS's default in JOIN.bat for double-click joining.
5. Wait until the client is connected before choosing PLAY GAME.
   Campus/guest Wi-Fi may block connections between devices.

CONTROLS
WASD move | Mouse look | Shift sprint | Ctrl sneak | C change perspective
F steal | Tab inventory | E escape (team loot must reach $10,000)

This is a Development build, as required by Lab 13.
The console and stat unit are available. MainMenu replaces the lab's Lobby;
PLAY GAME replaces T; DemoMap replaces Arena. The networking uses a listen
server, direct IP joining and server-controlled map travel.

If Windows reports a missing Visual C++ runtime, run the prerequisite installer
included under Engine/Extras/Redist when present, or install Microsoft's
Visual C++ x64 Redistributable from Microsoft.
