# Sneak animations

Hold Left Ctrl to crouch; release to stand. Movement uses the existing replicated
CharacterMovement crouch state, including remaining crouched under a low ceiling.

The five Epic Manny clips (idle, forward, backward, left, right) were copied from
UE 5.7 MoverExamples into Content/Characters/Mannequins/Anims/Sneak. Runtime assets
use the project skeleton and do not require enabling Mover or MoverExamples.
Existing IK retargeters produce matching Ninja and John clips in their Sneak folders.

ABP_Unarmed, ABP_Ninja and ABP_John inherit SneakAnimInstance. Their original
locomotion feeds a two-way blend before the existing foot IK. The crouch blend
space uses local velocity direction and speed (0–200 cm/s), with an approximately
0.17-second blend when entering or leaving crouch. Root motion is disabled and
root lock enabled for walking (disabled for crouch idle); CharacterMovement remains responsible for network movement.
Footsteps continue using the existing movement-driven audio system.

InstallSneakAnimations.py installs these assets with -EnablePlugins=MoverExamples.
Original animation blueprints are backed up in Saved/SneakAnimationBackup.
ValidateSneakAnimationsPIE.py checks crouch replication, lowered head pose, animated
walking feet, and restoration of standing locomotion in a two-player PIE session.

Validation: ArenaGameEditor Development build passed. Two-player PIE passed on both
network worlds: plugin-independent assets, crouch idle head lowering, animated
crouch walking feet, and return to standing (Saved/Logs/SneakAnimationValidationFinal.log).

Idle grounding correction:
Ninja/John use Mixamo skeletons with Hips as their first bone. Forcing root lock
on the idle clip replaced the authored low hip translation with its standing
reference translation, raising the feet roughly half a metre. Crouch idle now
has Force Root Lock disabled on all three skeletons; root motion remains disabled.
Walking clips, capsule placement, mesh offsets and the original foot IK are unchanged.
FixSneakIdleGrounding.py applies this correction to existing assets; the installer
also preserves it. The PIE regression checks ankle heights on the flat fixture floor
when first crouching and after stopping, along with network poses and animated feet.

Grounding regression passed in Saved/Logs/SneakGroundFixed.log: both network worlds,
initial idle, stop after walking, animated crouch walk, and standing restoration.
