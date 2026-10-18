# Detection and pursuit

Alert is server-authoritative and tracked separately for each player.
Visible players fill detection in `AlertFillDuration` (default 3 seconds).
Partial detection decays in `AlertDecayDuration` (default 2 seconds) after sight is lost.
Confirmed detection holds at 100% during `LostTargetChaseDuration` (default 3 seconds)
after both sight and close movement sensing are lost. During this bounded grace
period AI still chases the actual player, including sprinting or crouching players.
After grace expires, alert decays over `AlertSearchDuration` (default 6 seconds).
Reappearing before confirmed memory expires restores full detection immediately.

`TargetActor` is assigned to a confirmed currently sensed player, or a confirmed
player within the three-second chase grace. After grace, the actor target and
focus clear and the last tracked location is stored in `InvestigateLocation`. The existing Behavior Tree's TargetActor and
InvestigateLocation decorators abort both directions, so the guard switches from
chasing the actor to searching the location. Hidden movement is tracked only during the bounded grace period; afterward it
never updates that saved location. A different confirmed, visible teammate can remain the chase target.

Newly heard sounds, damage and team alerts can refresh investigation at their
reported position. Expired/unsuccessful stimuli do not restart investigation.
When the search timeout expires, pending investigation/damage locations are
cleared so an unreachable MoveTo does not keep the guard searching indefinitely.
Escaped and destroyed player pawns remain excluded from targeting.

Tune these durations on BP_ArenaAIController under AI Alert. No Behavior Tree
asset edits are needed. `Scripts/ValidateAIAlertPIE.py` exercises the controller with
controlled sight notifications and transient PIE actors; none of its actors or configuration changes
are saved to the level.

Validation: Development editor build passed. Controlled sight-event PIE checks passed
partial decay, confirmed target release, unchanged last-known location while hidden,
confirmed memory/search expiry, loss outside sight range, reacquisition, and switching
to a visible co-op teammate. These exercise the actual server controller and Blackboard,
with real network HUD notifications queued outside Python's editor RPC guard.

Validation limit: automatic Sight stimuli did not arrive in the offscreen fixture,
even with enabled sight, radius 1200, angle 30, hostile affiliation and an unobstructed
visibility trace. End-to-end world occlusion was therefore not validated by that run.
The final test uses explicit sense notifications, not automatic line-of-sight detection.
Rider debugging was attempted; the breakpoint did not bind ('No executable code is
associated with this line'), the paused GameThread stack was unknown and locals were
empty. Agent breakpoints were removed and the debugger session was stopped.

Visible players within `InstantDetectionDistance` (default 400 cm / 4 m)
are confirmed at 100% on the next AI update, including players who enter that
range while already partially detected. This requires current sight; proximity
alone does not detect an unseen player through this sight-based shortcut. The
independent 2m movement sensor below also detects nearby players outside the cone.
Zero disables the sight-based close-range shortcut.
Distance is measured in 3D between pawn origins. Normal sight loss and search
expiry remain active after close-range confirmation. Tune this property in
BP_ArenaAIController > AI Alert.

Close-range regression passed with controlled sight notifications in two-player PIE:
500 cm filled gradually; entering 200 cm confirmed immediately; first seeing a player
at 200 cm jumped from 0 to 100%; being close without sight did not detect; losing
sight still cleared confirmation/search. Test: `Scripts/ValidateCloseDetectionPIE.py`.

Current sight cone: PeripheralVisionAngleDegrees = 80 (80 degrees either side,
160 degrees total). SightRadius stays 1200 cm; LoseSightRadius stays 1500 cm.

Moving sprint detection:
A currently visible player who is sprinting and moving faster than 20 cm/s is
confirmed immediately within InstantSprintDetectionDistance (default 700 cm / 7 m).
The normal close confirmation remains 400 cm / 4 m. Holding sprint while stationary
keeps the normal radius. Sprint is read on the authoritative server; the existing
sprint RPC supplies the state for remote players. Both radii use 3D pawn distance.
Sight is required; hearing still investigates sounds using its existing behavior.
Sight loss still clears the actor target and lets alert/search memory expire.
The extended distance can be tuned on BP_ArenaAIController under AI Alert.
ValidateSprintDetectionPIE.py uses controlled sight events plus actual pawn movement
for normal movement, stationary sprint, outside/inside 7 m, and losing sight.

Sprint regression passed (Saved/Logs/SprintDetectionValidationFinal.log): normal
movement and stationary sprint at 6.5 m remained partial; moving sprint outside
7 m remained partial, inside 7 m selected a confirmed chase target; sight loss
expired the alert and an unseen sprint did not detect. Development editor build passed.


360-degree close movement:
NearbyMovementDetectionDistance defaults to 200 cm / 2 m, measured between pawn
origins in 3D. The server checks every player, including those outside the 160-degree
sight cone. A non-crouched player moving faster than 20 cm/s immediately fills the
alert and becomes a chase target when a Visibility trace from the guard's eyes to
the player's body is unobstructed. Walls and intervening floors block this sensor.
Unknown stationary players and crouching players do not trigger it. A player already
confirmed nearby remains detected when stopping; leaving the radius or becoming
occluded releases the proximity target and searches the last observed location.
Normal sight can independently retain the target. Existing alert expiry still applies.
Nearby detection uses its own state rather than fabricating a Sight stimulus.
ValidateNearbyMovementPIE.py exercises actual motion and actual wall collision with
Sight/Hearing disabled: behind the guard, inside/outside radius, crouch, stopping,
leaving, expiry and sprinting. Actors and collision changes are transient PIE-only.

Nearby movement validation passed (Saved/Logs/NearbyMovementValidation.log):
walking and sprinting behind the guard inside 2m immediately chase; actual wall
collision blocks detection; stationary/crouching and outside-radius checks passed;
leaving releases the target, searches the last location and expires. Build passed.


Three-second confirmed pursuit:
LostTargetChaseDuration defaults to 3 seconds. Every currently sensed update of a
confirmed target refreshes its deadline; pursuing a hidden player does not refresh
it. Sprinting/crouching does not cancel grace. After the deadline the target actor
is released, the last tracked point is frozen for a six-second search, and normal
alert decay resumes. A confirmed currently sensed teammate has priority over a
hidden grace target. Partial detection does not receive this grace period.
ValidateChaseGracePIE.py uses controlled sight events and actual sprint/crouch motion
to check bounded pursuit, frozen search location, normal expiry, and partial alerts.

Chase grace validation passed (Saved/Logs/ChaseGraceValidation.log): target remained
confirmed while sprinting and crouching unseen, actor pursuit ended after three
seconds, the search location froze, alert/search expired, and partial detection
did not receive grace. Development editor build passed.

Partial-cover visibility:
ArenaGameCharacter now implements IAISightTargetInterface. The engine calls its
CanBeSeenFrom method after the normal range/cone gate. Instead of testing only
actor center, it tests capsule-relative head, upper-torso and center points against
Visibility using complex collision. Any exposed point counts as visible. Crouching
shrinks the sample heights; capsule-based samples work without offscreen bone
animation on dedicated servers. The 2m close movement sensor reuses the same check.
Full cover still blocks all samples. The existing 3s chase grace remains separate
from whether the player is currently visible.
ValidateLowCoverSightPIE.py invokes the actual sight interface against real collision:
center ray blocked but exposed standing head visible, crouched low cover hidden,
full wall hidden, and a real SM_Sofa_2 from Office Pack. This tests occlusion queries
directly; automatic perception scheduling/HUD is not asserted by that test.

Low-cover validation passed (Saved/Logs/LowCoverSightValidationFinal.log): the old
center ray was blocked while the head sample was visible; crouching behind low
cover and standing behind a full wall were hidden. Office Pack SM_Sofa_2 measured
approximately 104 cm high and allowed the exposed standing head. Build passed.

Hearing reaction correction:
Real sprint-footstep reproduction confirmed the listener received noise and set
InvestigateLocation, but the controller had no focal point. Guards now rotate
smoothly toward controller desired rotation, including while stationary. Idle
investigation assigns a gameplay focal point at the latest heard location and
clears it when investigation ends. Confirmed pursuit (including chase grace) and
damage investigation retain priority; noise focus cannot override those branches.
BT_ArenaAI's noise MoveTo observes blackboard updates and its noise decorator
restarts on value changes, so fresh footsteps interrupt stale investigation waits.
ConfigureHearingReaction.py saves those settings using an editor-only helper.
ValidateHearingReactionPIE.py generates actual sprint movement/footsteps with
Sight disabled and the brain paused to isolate facing without relying on navmesh.
It checks the real hearing callback's Blackboard location, focal point and body yaw.
Full path traversal is not validated by this isolated facing test.

Hearing facing validation passed (Saved/Logs/HearingReactionValidation.log): real
sprint footsteps reached the hearing callback, set the latest investigation/focal
point, and rotated the stationary guard toward the source. This reproduces the
previous case that had a valid investigation point but no focal point. The editor
configuration helper verified and saved both Behavior Tree settings. Build passed.

Unreachable-player pursuit:
BT_ArenaAI uses BTTask_PursuePlayer for its TargetActor MoveTo. The task accepts
partial paths without requiring the target endpoint to be on the guard navmesh.
It walks to the reachable boundary, keeps gameplay focus on the confirmed player,
and remains running when pathfinding fails or a partial path ends. Idle requests
retry every 0.75 seconds; moving actor paths track the target without being reset
on every tick. Switching player targets cancels the old path immediately.
Only proximity to the actual player (140 cm with line of sight) completes the move
and allows the existing melee task to run. Blackboard removal/branch abort stops
movement and releases focus; the existing three-second sight-loss grace and search
logic remain authoritative. This does not shrink the guard or change the sofa gap.
Scripts/ConfigurePursuit.py installs the task and updates its editor graph reference.
Scripts/ValidatePursuitPIE.py exercises the real tree with controlled sight events:
failed navigation, resumed movement, partial paths, and loss-of-sight release.
Build passed. Saved/Logs/PursuitValidationVerified.log contains all four passing
PIE assertions, including a real partial navigation path on DemoMap. Sight events
are injected to isolate navigation; this does not assert automatic perception at
the user's exact sofa gap. UE emitted a handled navigation-system CDO ensure while
the Python test selected a reachable location; movement/path and grace assertions
continued and passed, and the editor exited normally.
