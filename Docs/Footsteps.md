# Player and guard footsteps

AArenaGameCharacter emits footsteps on the server while moving on the ground.
A lightweight unreliable multicast plays each step on nearby clients and the
listen host; dedicated servers do not create playback components. Local owners
use the same event, avoiding doubled owner/predicted audio.

The current default clips are the user's `left.mp3` and `right.mp3`, imported as
`/Game/Audio/Footsteps/SW_FootstepLeft` and `SW_FootstepRight`. Original MP3s are
retained in `Content/Audio/Footsteps/Source`. `Scripts/ImportUserFootsteps.py`
uses Unreal's MP3 decoder to export mono 16-bit PCM WAV, trims outer silence with
20 ms leading / 60 ms trailing margins, and applies a shared gain to match the
old placeholder impact level while preserving left/right relative loudness.
The prepared WAV sources are retained for reimport. Constructor references ensure
cooked builds include both clips and `SA_Footsteps`.

FootstepSounds entry 0 is left and entry 1 is right. A single custom clip can serve
both feet. The obsolete generated samples and their import script have been removed after
checking asset referencers and player/AI defaults.

Playback alternates left/right offsets near the capsule's feet, selects the matching
left/right clip and varies pitch slightly. Cadence follows actual horizontal
speed using FootstepInterval (0.4 seconds at normal walk speed), clamped to avoid
extreme rates. Sprint volume is 1.25x, sneak volume 0.18x of FootstepVolume (0.75).
AI uses the same walking system. Airborne, stationary, dead and escaped characters
emit no new steps. Playback follows the actor, auto-destroys and stops on actor
destruction. Timing is movement-driven, not tied to animation foot contacts.

SA_Footsteps spatializes mono audio, with a 120 cm inner radius and 1680 cm falloff
(total 18 m). Visibility occlusion reduces volume to 0.35 and low-passes at 1800 Hz.
Tune FootstepSounds, FootstepAttenuation and FootstepVolume under Audio > Footsteps
on either the player or AI Character Blueprint defaults.

AI hearing remains separate from audible playback: normal player footsteps report
FootstepRange (900 cm), sprinting uses SprintFootstepRange (1800 cm). Sneaking
retains the existing no-noise-stimulus behavior while playing quiet audible steps.
Guard footsteps do not report investigation noise, preventing guards investigating
each other's walking sounds.

Validation script: `Scripts/ValidateFootstepsPIE.py` drives host, client and guard
movement in a transient replicated test area, checks real spatial AudioComponents
on both network worlds, sprint/sneak volume and cadence, and no new steps while
stationary or airborne. No fixture actors or character settings are saved.

Latest Development editor build and two-player listen-server PIE validation passed: spatial components for host/client/AI appeared in both worlds, sprint was louder/faster, sneak was quieter, and stationary/airborne states emitted no new steps. Packaged builds and cross-machine latency were not tested.

User clip validation passed in two-player listen-server PIE: every observed footstep used SW_FootstepLeft or SW_FootstepRight, and each clip matched the sign of its left/right foot offset on both worlds for player and guard playback. Sprint/sneak and stationary/airborne checks passed with these prepared recordings.
