"""Preserve the authored pelvis translation for stationary crouch poses.
Mixamo skeletons use Hips as their root; forcing root lock raises their crouch.
"""
import unreal as u
for folder in ['/Game/Characters/Mannequins/Anims/Sneak','/Game/Characters/Ninja/Sneak','/Game/Characters/John/Sneak']:
    path=folder+'/MM_Unarmed_Crouch_Idle'
    clip=u.load_asset(path)
    assert clip,path
    clip.set_editor_property('enable_root_motion',False)
    clip.set_editor_property('force_root_lock',False)
    assert u.EditorAssetLibrary.save_loaded_asset(clip),path
    u.log('SNEAK_IDLE_GROUND_FIXED '+path)
