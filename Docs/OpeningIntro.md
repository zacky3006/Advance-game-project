# Opening video intro

Play Game plays `Content/Movies/OpeningJob.mp4` with its picture and audio.
The current clip is the supplied `2 mens.mp4`. Before playback, the menu fades
to opaque black over 0.9 seconds using smoothstep easing; media opens after a
brief 0.08-second black hold, so audio starts after the fade is complete.
The picture fits the viewport without stretching and uses black letterboxing.
Subtitles are supplied by the video itself; the old estimated text cues are disabled.
Space skips after a 0.4-second input guard. Audio completion opens DemoMap.
Stopping PIE or skipping closes the player and destroys its sound component.
Media errors fall through to the map; a 10-second opening timeout avoids a stuck screen.

The supplied MP4 is 1920x1080 and approximately 18.219 seconds long. Movies are staged as
loose NonUFS files by DefaultGame.ini so the native media backend can open them
in packaged builds. Windows uses Unreal's enabled-by-default WMF backend.
`M_IntroVideo` is a UI material with a Color texture parameter named Video,
matching UE 5.7's new-style media texture output.
The widget owns a transient media texture and material instance. The material is
created by `Scripts/CreateIntroVideoMaterial.py` and referenced by the widget
constructor so it is included when cooking.

Validated in PIE: the supplied clip renders its embedded English subtitle;
media reports one audio and one video track and duration 18.218 seconds.
The active sound component produces a nonzero audio envelope (~0.070).
Natural completion travels to DemoMap and removes the menu. Calling the same
FinishIntro handler used by Space also travels to DemoMap and leaves zero media
sound components in the destination world. Physical keyboard input and a packaged
build were not tested.
