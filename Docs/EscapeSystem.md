# Escape objective

Team money must reach at least $10,000. Network games support one or two connected
players; standalone PIE can still be used for solo iteration. F steals loot,
and E extracts when a living player is within 200 cm with a clear visibility
trace. All pickup and extraction decisions are validated by the server.

The Escape Pad glows red below the shared threshold and green at or above it.
The green overlay disables depth testing, so it remains visible through walls.
The pad is translucent, thin and noncolliding. Its nearby prompt shows the
current/required team money while locked, or [E] ESCAPE when unlocked.

The first escaped player becomes invisible and noncolliding, cannot interact
or attack, and spectates the remaining teammate. The team HUD displays
ESCAPED 1/2 and PLAYING 1. The ending starts once everyone has escaped or died, provided at least one player escaped.

The ending fades to black, plays Content/Movies/Ending.mp4 (the supplied
2 mens end.mp4, 1920x1080, 16.811 seconds), then opens the run summary. Missing
or failed media skips to the summary. Space skips the ending movie.

The summary groups matching items without pictures and shows unit value,
quantity, subtotal, total pieces, shared total value and the server's elapsed
run time. The snapshot is taken when the final teammate escapes or dies, excluding
the ending fade/movie and result-screen time. FINISH marks each player ready;
the first waits for the other. When both confirm, ServerTravel returns the
party to MainMenu, preserving its connection and port. A new run resets loot,
money and escape status.

AEscapePoint can be placed next to a window or anywhere else in a map. Details
exposes Mesh, Required Value, Interaction Range and Ending Movie. Movie names
refer to files inside Content/Movies, which is staged by packaging settings.
Scripts/CreateEscapePad.py creates the glow materials. See Multiplayer.md for
connection instructions and test coverage.

Deaths: dead players spectate; completion counts escaped + dead, with at least one escape required. Results show ESCAPED / DEAD. Solo death and all-dead co-op show the black/red Mission Failed screen before returning to MainMenu. AI hits kill players in one hit.
