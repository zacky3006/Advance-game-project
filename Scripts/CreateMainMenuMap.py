"""Create the dedicated menu map only; existing gameplay maps are not edited."""
import unreal

menu_path = "/Game/Maps/MainMenu"
if not unreal.EditorAssetLibrary.does_asset_exist(menu_path):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if not world:
        raise RuntimeError("Could not create MainMenu")
else:
    world = unreal.EditorLoadingAndSavingUtils.load_map(menu_path)
mode = unreal.load_class(None, "/Script/ArenaGame.MainMenuGameMode")
if not mode:
    raise RuntimeError("Build ArenaGameEditor before creating MainMenu")
world.get_world_settings().set_editor_property("default_game_mode", mode)
if not unreal.EditorLoadingAndSavingUtils.save_map(world, menu_path):
    raise RuntimeError("Could not save MainMenu")
unreal.log("MAIN_MENU_READY " + menu_path)
