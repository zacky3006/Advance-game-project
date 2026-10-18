# Windows Development package

Run `Scripts/Packaging/PackageDevelopment.ps1` in PowerShell to build, cook,
stage, and archive the Windows Development game using Unreal Engine 5.7.
The script copies `HOST.bat`, `JOIN.bat`, and `README.txt` beside the executable
under `Builds/Development/Windows`. Distribute that entire Windows folder.
The internal executable name remains `ArenaGame.exe`; the game title is Dirty Work.
If local PowerShell script execution is disabled, invoke it for this process only:
`powershell.exe -NoProfile -ExecutionPolicy Bypass -File Scripts/Packaging/PackageDevelopment.ps1`.
End users only need the executable/BAT files, not this packaging script.

`HOST.bat` opens `/Game/Maps/MainMenu?listen` on port 7777.
`JOIN.bat` joins `127.0.0.1` by default. For a separate PC on the same LAN, run
`JOIN.bat 192.168.1.10` with the host's actual IPv4 address. The map/address is
the first executable argument, before the window/resolution options, as in Lab 13.
Wait for the client to connect before choosing Play Game. MainMenu replaces the
lab's Lobby, Play Game replaces T, and DemoMap replaces Arena.

Opening `ArenaGame.exe` without arguments supports solo play from MainMenu.
An existing network party travels together through the intro into DemoMap.
The package includes the opening/ending movies and the cooked gameplay maps.
The native default extraction requirement and the saved DemoMap point use $10,000.

The Editor-only thumbnail automation test is guarded by both `WITH_EDITOR` and
`WITH_DEV_AUTOMATION_TESTS` so packaged Development builds do not compile its
Editor-only asset compilation calls. Gameplay is unchanged by that guard.

The first package log is `Saved/Logs/PackageDevelopment.log`.
Build output is ignored by Git because it contains generated binaries and cooked data.
The launchers cap each process at 60 FPS. IpNetDriver initial/connection timeouts
are 120 seconds: the first packaged run took roughly 22 seconds to load DemoMap,
and the original client timed out at 20 seconds. The updated network config was
restaged and verified by extracting DefaultEngine.ini from the final .pak.

## Actual packaged validation (2026-10-18)

- BuildCookRun succeeded (Development game and Editor compiled; cook, stage,
  pak/IoStore and archive completed). The initial package took about 15 minutes.
- The root executable opened MainMenu without Unreal Editor and exited normally.
- The actual HOST.bat opened MainMenu listening on UDP port 7777.
- The actual JOIN.bat opened a separate game process and joined 127.0.0.1:7777.
- Play Game initiated server travel; host loaded DemoMap at 14:01:26 UTC and
  client loaded it at 14:01:30 UTC, then the server logged that client's join.
- Both idle players were killed by AI. Mission Failed appeared, and both returned
  to MainMenu (14:01:47/48 UTC) with the client rejoining the same host.
- Both processes stayed responsive and were closed normally after the test.
  Final run logs contain no NetworkFailure or fatal error.

Evidence: Saved/Logs/PackagedHostValidated.log,
Saved/Logs/PackagedClientValidated.log, Saved/PackagedSoloMenu.png and
Saved/PackagedClientGameplay.png (the latter captures the Mission Failed state).
Generated test saves/logs were removed from the distributable after preserving
this evidence outside it. Windows folder size is approximately 1.9 GiB.

Limits: this validates real loopback joining, map travel, AI damage, team failure
and return-to-menu, not every gameplay interaction. Separate physical PCs/campus
Wi-Fi were not tested. The dual-process run showed a nonfatal virtual shadow map
non-Nanite marking-queue performance warning; low-end/dual-process performance
still needs profiling. Its presence does not indicate a network failure or crash.
