# Escape objective

Collect at least $1,000, approach the circular glowing Escape Pad and press E.
F still steals loot. The point shows the required/current amount when nearby
and locked. Below $1,000 the translucent pad glows red; at $1,000 or above it
turns green and gains a green translucent overlay with depth testing disabled,
so its silhouette remains visible through walls. The thin pad has no collision
and can be walked over. Emissive materials and Fresnel brighten its rim. Interaction
still requires a living player within 200 cm and an unobstructed visibility trace.
Server-side validation rechecks money, distance, health and line of sight.
Repeated inputs and collecting/opening inventory during completion are blocked.

The ending overlay fades to black over one second, then plays
`Content/Movies/Ending.mp4`, then shows the run summary. Until an ending clip is
supplied, it shows the summary after fading without playing the intro clip.
Space can skip the future ending movie to the summary. Movie failure or opening
timeout also opens the summary. FINISH at the bottom right returns to
`/Game/Maps/MainMenu`; no automatic travel occurs while viewing results.

The summary uses the inventory's dark/gold/green theme without item pictures.
Rows group matching name, unit value, mesh and materials just like the Tab bag,
and show name, unit value, quantity and subtotal, sorted by descending subtotal.
Top cards display total pieces, total value and elapsed time (mm:ss, or hh:mm:ss
for longer runs). Inventory and elapsed DemoMap world time are copied at
extraction, excluding the opening movie, pause time, ending fade, ending movie
and time spent on results. Input remains locked until returning to the menu.

`AEscapePoint` can be dragged into other levels. Its Mesh, Required Value,
Interaction Range and Ending Movie can be configured in Details. The movie field
accepts a filename within Content/Movies. Existing movie packaging settings also
stage the future ending file. `Scripts/PlaceEscapePoint.py` places one example
near DemoMap's existing player pawn and saves only that map.

Current scope: the existing per-player loot inventory in one gameplay session.
Completing a run returns that local player to the menu; this is not a shared
multiplayer match victory or persistent save system.

Validation: ArenaGameEditor Development build passed. In PIE, $999 kept the
highlight hidden and produced no ending widget; $1,000 enabled the green
highlight. Attempts from 500 cm away and through a temporary opaque blocker
were rejected. The green silhouette was visually confirmed through that
blocker. Removing the blocker and interacting nearby created one ending
overlay and returned to MainMenu with one menu widget. Test inventory and
blocker changes existed only in PIE. Physical E input, networked clients and
the future ending clip have not been tested.

Pad update validation: Development build passed; in PIE the material parameter
was red at $0 and green at $1,000, with the green overlay enabled at the threshold.
The placed pad is 140 cm wide and 8 cm thick at Z=5, has no collision, and still
starts the ending sequence when interacting nearby. The red translucent appearance
was visually inspected. `Scripts/CreateEscapePad.py` creates the pad materials.

Summary validation: Development build passed. PIE test inventory containing one
$800 Laptop and two $250 Gold Watches displayed two grouped rows, 3 total pieces
and $1,300 total, with a frozen 00:08 duration and no thumbnails. Results stayed
in DemoMap until FinishClicked, which opened MainMenu with one menu widget.
A second PIE-only test used OpeningJob.mp4 as a temporary ending: results were
absent during playback, appeared after natural completion, and media sound
components were cleaned up. This override was not saved to the map; Ending.mp4
is still the configured future ending clip.

Ending clip supplied: `2 mens end.mp4` is now installed as
`Content/Movies/Ending.mp4` (1920x1080, 16.811 seconds). PIE validation confirmed
the configured file plays with one video/audio track, visible embedded subtitles
and a nonzero audio envelope. Natural completion opens one run summary, and
FINISH returns to MainMenu. No source/build changes were needed for this asset.
