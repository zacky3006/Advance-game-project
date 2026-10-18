# Opening video intro

Play Game plays `Content/Movies/OpeningJob.mp4` with its picture and audio.
The current clip is the supplied `2 mens.mp4`. Before playback, the menu fades
to opaque black over 0.9 seconds using smoothstep easing; media opens after a
brief 0.08-second black hold, so audio starts after the fade is complete.
The picture fits the viewport without stretching and uses black letterboxing.
Subtitles are supplied by the video itself; the old estimated text cues are disabled.
Play Game requests the server to start the intro on every connected player's menu.
After a 0.4-second input guard, Space casts one skip vote per player. The prompt
shows votes / current participants; repeated votes do not count. Unanimous votes
skip for the whole party. A solo player needs one vote. Natural video completion
or a local media failure marks that participant finished; everyone finishing also
starts DemoMap. A participant that finishes early waits on black for teammates.
Disconnects remove that participant from the vote/finished roster. A player joining
during the intro starts their own playback and joins the roster.

Before server travel, all local menu controllers close their menu/media, clear
move/look input blocks, hide the cursor and restore GameOnly input. The gameplay
controller repeats input restoration in BeginPlay so a nonseamless travel cannot
carry UIOnly viewport state into the game. This fixes the other player being unable
to walk when one player skips. Media open errors and a 10-second opening timeout
report local completion instead of independently travelling the party.

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
Original solo playback validation confirmed media/sound cleanup. Multiplayer regression
coverage is in `Scripts/ValidateIntroSkipPIE.py`: client-first and host-first votes,
one-vote/duplicate-vote rejection, natural completion, both local viewport input
modes, menu cleanup, and actual movement of both characters. Physical keyboard
input and a packaged build have not been tested.

Latest listen-server + client PIE regression passed all three cases: client votes first, host votes first, and natural playback completion. Both viewports accepted gameplay input, menus were removed, and both characters moved in every case. Single votes and duplicate votes did not skip a two-player intro.
