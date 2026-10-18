"""Install the persistent, partial-path chase task in the guard behavior tree."""
import unreal as u
tree = u.load_asset('/Game/_Student/AI/BT_ArenaAI')
assert u.CoopPIETestLibrary.configure_pursuit_tree(tree)
assert u.EditorAssetLibrary.save_loaded_asset(tree, only_if_is_dirty=False)
u.log('PURSUIT_TREE PASS: installed persistent partial-path chase')
