"""Update noise investigation to react to newly heard locations."""
import unreal as u
tree=u.load_asset('/Game/_Student/AI/BT_ArenaAI')
assert u.CoopPIETestLibrary.configure_hearing_tree(tree)
assert u.EditorAssetLibrary.save_loaded_asset(tree,only_if_is_dirty=False)
u.log('HEARING_TREE PASS: MoveTo observes new locations; noise changes interrupt waits')
