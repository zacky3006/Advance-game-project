# Two-player cooperative heist

The server owns the shared team inventory and extraction roster. Both players
see the same Team Haul total and Tab inventory. A pickup can only be collected
once: server reach/visibility/health checks run before adding it to GameState and
destroying the replicated pickup. Joining clients receive the current inventory.

F steals, E extracts. The team needs at least $10,000 and at least one connected player. Solo listen-server and standalone play are supported.
The first extraction or death seals the roster; further joins are rejected. An escaped
player's pawn is hidden, immobile, noncolliding and immune to damage. That player
watches the remaining teammate and cannot collect or attack. AI no longer
targets escaped players. The HUD shows ESCAPED 1/2 and PLAYING 1.

Once all players have escaped or died, at least one escape starts the ending clip for all clients, carrying the
same inventory snapshot and server run time. Each client can skip its own clip
to the summary. FINISH marks that player ready; the button then shows WAITING
x/2. After both confirm, the server travels the whole party to MainMenu.
Play Game in the menu starts a listen server when playing alone, or requests
server travel for an existing party (including a dedicated server).

## Local testing

Open DemoMap in Unreal. Play settings: Number of Players 2, Net Mode Play As
Listen Server (or Play As Client with a dedicated server), Run Under One Process.
DemoMap has two PlayerStarts; the old auto-possessed placed player was replaced
with normal GameMode spawning. Each window should show a different character.

For separate instances, host starts from MainMenu and presses Play Game. Join
from another instance's Unreal console with `open 127.0.0.1:7777` on the same
machine, or `open HOST_LAN_IP:7777` on the same LAN. This is direct IP joining;
there is no online lobby/discovery service or join-address UI yet.

A dedicated server can be run using the installed editor executable:

```powershell
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' 'D:/AGD/AGD_ProjectSteal/Advance-game-project/ArenaGame.uproject' /Game/Office_Pack_Vol_1/Maps/DemoMap -server -log -port=7777
```

This does not require a packaged dedicated-server target. Packaging a standalone
server executable requires a suitable server-capable Unreal build/toolchain.

Team size is limited to two on gameplay admission. A disconnect after extraction
has begun aborts the sealed run and returns remaining players to the menu, rather
than silently counting a disconnect as an escape. A dead player spectates the surviving teammate. One escape plus one death succeeds and the summary displays both counts. Only an all-dead team shows MISSION FAILED, with a solid black background and red text fading in, before returning to the menu. Solo death uses the same failure presentation. The server waits five seconds before requesting menu travel. Positive AI damage kills a player in one hit; no revive system has been added.

Network heists use extraction as the victory condition; the old kill-score win
condition does not finish a cooperative run.

## Validation

Development editor build passed. Two-player listen-server PIE verified distinct
replicated player characters, shared pickup metadata and a shared inventory.
Competing pickup requests awarded one $600 item once and removed it from both
worlds. A client-only pickup added $450, giving both players the same $1,050.
Extraction below $1,000 was rejected. The first client escaped at 1/2 without
starting the ending; its camera matched the remaining teammate's FollowCamera.
The final extraction started the ending on both clients, followed by both
summaries. One FINISH waited for the other; the second returned both to the menu.
Starting another round preserved the connection and reset money and escapes.

`Scripts/ValidateCoopPIE.py` runs this integration sequence using the editor's
saved two-player listen-server PIE settings. Its development-only native timer
bridge is needed because Unreal's Python editor execution guard forces RPCs to
execute locally; actions queued for the next native game tick use real RPCs.
LAN/Internet sessions, packaged builds and dedicated-server execution have not
been run. A debugger attempt was blocked by missing executable native symbols;
the integration checks above use live replicated data, camera positions and logs.

Death validation: `Scripts/ValidateCoopDeathPIE.py` uses a queued native one-point AI hit. It checks lethal damage, destroyed victim pawn, replicated death count, spectator camera alignment, 1 escaped / 1 dead completion, results on both clients, both FINISH, replay reset, and all-dead failure returning both clients to the menu. `Scripts/ValidateMissionFailedPIE.py` checks the new fade-in failure screen in solo and two-player sessions; `ValidateSoloDeathPIE.py` invokes this validation for solo runs.

Latest death checks passed in listen-server PIE: one AI hit killed the client, the client spectated the survivor, 1 escaped / 1 dead finished both clients, and all-dead returned both to MainMenu. The earlier immediate solo return has been replaced by the Mission Failed presentation.

Opening cutscene: Play Game starts playback on all connected menu controllers.
Space votes to skip; all participants must vote, or all must finish playback.
The counter updates per client, duplicates are ignored, and solo play needs one
vote. The server restores GameOnly input and removes menus on every client before
travelling; gameplay BeginPlay also restores input to prevent UIOnly state leaking
across map travel. See `Docs/OpeningIntro.md` and `Scripts/ValidateIntroSkipPIE.py`.

First-person turning: the C camera toggle keeps camera activation and owner-only
mesh hiding local, but sends a reliable rotation-policy change to the server.
The replicated policy enables controller yaw and disables movement-facing rotation
on the server, allowing built-in CharacterMovement control rotation updates to
rotate a stationary client pawn. Returning to third person restores the saved
rotation settings. Mouse angle changes continue through Unreal movement networking;
there is no reliable RPC per mouse event. Regression: `Scripts/ValidateLookReplicationPIE.py`
checks client-to-server and host-to-client stationary yaw swings, then restoration
of third-person movement rotation. Test actor names are not assumed identical
across network worlds; player replicas are identified independently in each world.


Mission Failed presentation: the gameplay view fades to solid black over 0.7 seconds,
then the centered red MISSION FAILED label fades in from 0.7 to 1.9 seconds.
A reliable client RPC shows it to every teammate only when the entire team is dead.
One dead player with a living teammate continues spectating. One escape plus one
death remains a successful extraction. The BGM fades out and input is blocked while
the failure screen is visible. The server requests the shared menu travel after
five seconds; the network travel grace period can add time for multiplayer.

Mission Failed validation passed in solo and two-player listen-server PIE:
MissionFailedSoloFinal.log and MissionFailedCoopFinal.log. Tests confirmed one death
does not fail co-op, failure UI on both worlds, label fade opacity, full background
fade, BGM stopping, and both players reaching MainMenu. Build passed.
