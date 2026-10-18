"""Place one editable escape prop near the demo's player start; never duplicate it."""
import unreal

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap'):
    raise RuntimeError('Could not open DemoMap')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.EscapePoint)]
if existing:
    print('Escape point already placed:', existing[0].get_actor_location())
else:
    pawns = [a for a in actors.get_all_level_actors()
             if isinstance(a, unreal.Pawn) and 'BP_ThirdPersonCharacter' in a.get_class().get_path_name()]
    if not pawns:
        raise RuntimeError('Player pawn not found; no map changes made')
    pawn = pawns[0]
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    origin = pawn.get_actor_location()
    placement = None
    for offset in [unreal.Vector(240, 0, -85), unreal.Vector(0, 240, -85),
                   unreal.Vector(-240, 0, -85), unreal.Vector(0, -240, -85)]:
        target = origin + offset
        hit = unreal.SystemLibrary.line_trace_single(
            world, origin, target, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
            False, [pawn], unreal.DrawDebugTrace.NONE, True)
        if hit is None:
            placement = target
            break
    if placement is None:
        raise RuntimeError('No clear escape placement near player; no map changes made')
    point = actors.spawn_actor_from_class(unreal.EscapePoint, placement)
    point.set_editor_property('required_value', 10000)
    point.set_actor_label('Escape Point - $10000 - E')
    if not level.save_current_level():
        raise RuntimeError('Could not save DemoMap')
    print('Placed escape point:', point.get_actor_location())
