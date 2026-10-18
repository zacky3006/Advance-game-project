"""Import the supplied song and replace DemoMap's old HVAC bed with looping BGM."""
import unreal as u
from pathlib import Path

task = u.AssetImportTask()
task.filename = str(Path(u.Paths.project_content_dir()).resolve() / 'Audio/Music/Source/Scene Of The Crime.mp3')
task.destination_path = '/Game/Audio/Music'
task.destination_name = 'SW_SceneOfTheCrime'
task.automated = True
task.replace_existing = True
task.save = False
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = u.load_asset('/Game/Audio/Music/SW_SceneOfTheCrime')
assert sound
sound.set_editor_property('looping', True)
sound.set_editor_property('volume', .25)
assert u.EditorAssetLibrary.save_loaded_asset(sound)
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
found = [a for a in actors.get_all_level_actors() if 'OfficeHVAC' in a.tags or 'OfficeBGM' in a.tags]
assert len(found) <= 1, 'Duplicate background audio actors'
actor = found[0] if found else actors.spawn_actor_from_class(u.AmbientSound, u.Vector(0, 0, 200))
actor.set_actor_label('Office BGM - Scene Of The Crime (Loop)')
actor.set_editor_property('tags', ['OfficeBGM'])
audio = actor.get_component_by_class(u.AudioComponent)
audio.set_sound(sound)
audio.set_editor_property('auto_activate', True)
audio.set_editor_property('allow_spatialization', False)
audio.set_editor_property('override_attenuation', False)
audio.set_editor_property('volume_multiplier', 1.0)
assert level.save_current_level()
u.log('OFFICE_BGM_INSTALLED looping=True volume=.25 duration='+str(sound.get_editor_property('duration')))
