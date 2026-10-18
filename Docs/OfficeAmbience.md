# Office background music

DemoMap now uses /Game/Audio/Music/SW_SceneOfTheCrime, imported from the supplied
Scene Of The Crime.mp3. The original file is retained in Content/Audio/Music/Source.
SoundWave Looping is enabled; Volume is 0.25. One non-spatial AmbientSound actor
labelled Office BGM - Scene Of The Crime (Loop), tagged OfficeBGM, replaces the
previous OfficeHVAC actor. The HVAC sound no longer plays in the map.

The song repeats for the whole round, including spectating while a teammate is
still playing. Local player controllers fade it out over one second when the
team completes or fails, and before the ending sequence. Level teardown stops
it on return to the menu; a new run starts a fresh playback.

InstallOfficeMusic.py imports and installs it without duplicate actors.
ValidateOfficeMusicPIE.py checks playback, looping across the end of the track,
and stopping after round completion.

The unused HVAC SoundWave, WAV source, generator and importer were removed after
checking the SoundWave had no asset referencers. Active music and footsteps remain.

Validation: Development editor build passed. PIE confirmed active playback,
continued playback across the track end, and a completed round stopping BGM
(Saved/Logs/OfficeMusicValidation.log).
