"""Run with UnrealEditor -RenderOffscreen -ExecutePythonScript=<this file>.
All inventory, actor and health changes below are transient PIE test data.
"""
import unreal
import time
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
# Uses the editor's saved PIE settings: two clients, listen server, one process.
context = {'stage': 0, 'start': time.monotonic(), 'at': time.monotonic(), 'handle': None}

def tick(delta, ctx=context, ue=unreal, clock=time):
    now = clock.monotonic()
    try:
        worlds = ue.EditorLevelLibrary.get_pie_worlds(True)
        if now - ctx['start'] > 160:
            raise RuntimeError('Coop test timeout at stage ' + str(ctx['stage']))
        if len(worlds) < 2:
            return
        server = next((w for w in worlds if ue.GameplayStatics.get_game_mode(w)), None)
        client = next((w for w in worlds if not ue.GameplayStatics.get_game_mode(w)), None)
        if not server or not client:
            return
        if ctx['stage'] == 8:
            if 'MainMenu.' not in server.get_path_name() or 'MainMenu.' not in client.get_path_name():
                return
            if len(ue.WidgetLibrary.get_all_widgets_of_class(client, ue.MainMenuWidget, False)) != 1: return
            ue.log('COOP_TEST PASS: both players returned to MainMenu')
            ctx['stage'] = 9
            ctx['at'] = now
            ue.CoopPIETestLibrary.queue_action(ue.GameplayStatics.get_player_controller(server, 0), 'Start')
            return
        if ctx['stage'] == 9:
            if 'DemoMap.' not in server.get_path_name() or 'DemoMap.' not in client.get_path_name():
                return
            gs = ue.GameplayStatics.get_game_state(server)
            if not isinstance(gs, ue.ArenaGameState) or gs.get_editor_property('connected_players') != 2:
                return
            assert gs.get_team_total_value() == 0
            assert len(gs.get_editor_property('escaped_players')) == 0
            ue.log('COOP_TEST PASS: replay starts with shared $0 and zero escapes')
            ue.unregister_slate_post_tick_callback(ctx['handle'])
            ue.get_editor_subsystem(ue.LevelEditorSubsystem).editor_request_end_play()
            ctx['stage'] = 10
            ctx['handle'] = ue.register_slate_post_tick_callback(lambda dt, u=ue: u.SystemLibrary.quit_editor())
            return
        if 'DemoMap.' not in server.get_path_name():
            raise RuntimeError('Run ended unexpectedly: ' + server.get_path_name())
        gs = ue.GameplayStatics.get_game_state(server)
        cg = ue.GameplayStatics.get_game_state(client)
        pawns = [ue.GameplayStatics.get_player_pawn(server, i) for i in range(2)]
        for p in pawns:
            if p:
                p.set_editor_property('can_be_damaged', False)
        cp = ue.GameplayStatics.get_player_pawn(client, 0)
        if not all(pawns) or not cp:
            if now - ctx['at'] > 30:
                raise RuntimeError('Missing player pawn after 30 seconds; server pawns=' + str(pawns))
            return
        if ctx['stage'] == 0:
            for i, p in enumerate(pawns):
                p.set_actor_location(ue.Vector(-2370, 1020 + i * 180, 92.15), False, True)
            cp.set_actor_location(ue.Vector(-2370, 1200, 92.15), False, True)
            for w in worlds:
                point = ue.GameplayStatics.get_all_actors_of_class(w, ue.EscapePoint)[0]
                point.get_editor_property('mesh').set_mobility(ue.ComponentMobility.MOVABLE)
                point.set_actor_location(ue.Vector(-2370, 1110, 5), False, True)
            loot = ue.GameplayStatics.get_all_actors_of_class(server, ue.LootPickup)[0]
            ctx['pickup'] = loot.get_name()
            loot.get_editor_property('mesh').set_mobility(ue.ComponentMobility.MOVABLE)
            loot.set_actor_location(ue.Vector(-2370, 1110, 125), False, True)
            loot.set_editor_property('value', 600)
            loot.force_net_update()
            ctx['stage'] = 1; ctx['at'] = now
            ue.log('COOP_TEST: two distinct pawns connected, pickup prepared')
        elif ctx['stage'] == 1 and now - ctx['at'] > 1:
            sl = next(a for a in ue.GameplayStatics.get_all_actors_of_class(server, ue.LootPickup) if a.get_name() == ctx['pickup'])
            cl = next(a for a in ue.GameplayStatics.get_all_actors_of_class(client, ue.LootPickup) if a.get_name() == ctx['pickup'])
            assert cl.get_editor_property('value') == 600
            ue.CoopPIETestLibrary.queue_action(cp.get_component_by_class(ue.LootCollectorComponent), 'Collect', cl)
            ue.CoopPIETestLibrary.queue_action(pawns[0].get_component_by_class(ue.LootCollectorComponent), 'Collect', sl)
            ctx['stage'] = 2; ctx['at'] = now
        elif ctx['stage'] == 2 and now - ctx['at'] > 1:
            assert gs.get_team_total_value() == 600, ('pickup rejected or duplicated', gs.get_team_total_value())
            assert cg.get_team_total_value() == 600
            assert len(gs.get_editor_property('team_inventory')) == 1
            assert len(cp.get_component_by_class(ue.LootCollectorComponent).get_editor_property('inventory')) == 1
            assert not any(a.get_name() == ctx['pickup'] for a in ue.GameplayStatics.get_all_actors_of_class(client, ue.LootPickup))
            ue.log('COOP_TEST PASS: competing pickup requests award shared $600 once')
            ue.CoopPIETestLibrary.queue_action(cp.get_component_by_class(ue.LootCollectorComponent), 'Escape')
            loot = ue.GameplayStatics.get_all_actors_of_class(server, ue.LootPickup)[0]
            ctx['pickup'] = loot.get_name()
            loot.get_editor_property('mesh').set_mobility(ue.ComponentMobility.MOVABLE)
            loot.set_actor_location(ue.Vector(-2370, 1110, 125), False, True)
            loot.set_editor_property('value', 450); loot.force_net_update()
            ue.GameplayStatics.get_player_controller(client, 0).set_control_rotation(ue.Rotator(pitch=-5, yaw=-90, roll=0))
            ctx['stage'] = 3; ctx['at'] = now
        elif ctx['stage'] == 3 and now - ctx['at'] > 1:
            assert len(gs.get_editor_property('escaped_players')) == 0
            loot = next(a for a in ue.GameplayStatics.get_all_actors_of_class(client, ue.LootPickup) if a.get_name() == ctx['pickup'])
            server_loot = next(a for a in ue.GameplayStatics.get_all_actors_of_class(server, ue.LootPickup) if a.get_name() == ctx['pickup'])
            hit = ue.SystemLibrary.line_trace_single(server, pawns[1].get_actor_location() + ue.Vector(0,0,64), server_loot.get_actor_location(), ue.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [pawns[1]], ue.DrawDebugTrace.NONE, True)
            ue.log('COOP_TEST client pickup: server pawn=' + str(pawns[1].get_actor_location()) + ' client pawn=' + str(cp.get_actor_location()) + ' item=' + str(server_loot.get_actor_location()) + ' hit=' + (str(hit.to_tuple()[9]) if hit else 'none'))
            ue.log('COOP_TEST client component replicated=' + str(cp.get_component_by_class(ue.LootCollectorComponent).get_editor_property('replicates')) + ' role=' + str(cp.get_local_role()))
            ue.CoopPIETestLibrary.queue_action(cp.get_component_by_class(ue.LootCollectorComponent), 'Collect', loot)
            ctx['stage'] = 4; ctx['at'] = now
        elif ctx['stage'] == 4 and now - ctx['at'] > 1:
            assert gs.get_team_total_value() == 1050 and cg.get_team_total_value() == 1050, (gs.get_team_total_value(), cg.get_team_total_value())
            for w in worlds:
                point = ue.GameplayStatics.get_all_actors_of_class(w, ue.EscapePoint)[0]
                point.get_editor_property('mesh').set_mobility(ue.ComponentMobility.MOVABLE)
                point.set_actor_location(ue.Vector(-2370, 1110, 5), False, True)
            ue.CoopPIETestLibrary.queue_action(cp.get_component_by_class(ue.LootCollectorComponent), 'Escape')
            ctx['stage'] = 5; ctx['at'] = now
        elif ctx['stage'] == 5 and now - ctx['at'] > 1:
            assert len(gs.get_editor_property('escaped_players')) == 1
            assert len(cg.get_editor_property('escaped_players')) == 1
            assert not gs.get_editor_property('team_complete')
            assert cp.get_component_by_class(ue.LootCollectorComponent).has_escaped()
            assert len(ue.WidgetLibrary.get_all_widgets_of_class(client, ue.EscapeSequenceWidget, False)) == 0
            target = next(a for a in ue.GameplayStatics.get_all_actors_of_class(client, ue.ArenaGameCharacter)
                          if a != cp and a.get_class() == cp.get_class())
            follow = next(c for c in target.get_components_by_class(ue.CameraComponent) if c.get_name() == 'FollowCamera')
            actual = ue.GameplayStatics.get_player_camera_manager(client, 0).get_camera_location()
            expected = follow.get_socket_location('')
            assert abs(actual.x-expected.x) + abs(actual.y-expected.y) + abs(actual.z-expected.z) < 5, (actual, expected)
            ue.log('COOP_TEST PASS: first player spectates at 1/2, no ending starts')
            ue.CoopPIETestLibrary.queue_action(pawns[0].get_component_by_class(ue.LootCollectorComponent), 'Escape')
            ctx['stage'] = 6; ctx['at'] = now
        elif ctx['stage'] == 6 and now - ctx['at'] > 2:
            assert gs.get_editor_property('team_complete') and cg.get_editor_property('team_complete')
            assert len(gs.get_editor_property('escaped_players')) == 2
            for w in worlds:
                assert len(ue.WidgetLibrary.get_all_widgets_of_class(w, ue.EscapeSequenceWidget, False)) == 1
            if not ctx.get('ending_verified'):
                ue.log('COOP_TEST PASS: 2/2 starts ending sequence on both clients')
                ctx['ending_verified'] = True
            if now - ctx['at'] > 21:
                for w in worlds:
                    assert len(ue.WidgetLibrary.get_all_widgets_of_class(w, ue.RunResultsWidget, False)) == 1
                result = ue.WidgetLibrary.get_all_widgets_of_class(server, ue.RunResultsWidget, False)[0]
                ue.CoopPIETestLibrary.queue_action(result, 'Finish')
                ctx['stage'] = 7; ctx['at'] = now
        elif ctx['stage'] == 7 and now - ctx['at'] > 1:
            assert gs.get_editor_property('results_ready') == 1
            assert 'DemoMap.' in client.get_path_name()
            ue.log('COOP_TEST PASS: first FINISH waits for teammate')
            ue.CoopPIETestLibrary.queue_action(ue.WidgetLibrary.get_all_widgets_of_class(client, ue.RunResultsWidget, False)[0], 'Finish')
            ctx['stage'] = 8; ctx['at'] = now
    except Exception as ex:
        ue.log_error('COOP_TEST FAIL stage=' + str(ctx['stage']) + ' ' + repr(ex))
        ue.unregister_slate_post_tick_callback(ctx['handle'])
        ue.get_editor_subsystem(ue.LevelEditorSubsystem).editor_request_end_play()
        ctx['handle'] = ue.register_slate_post_tick_callback(lambda dt, u=ue: u.SystemLibrary.quit_editor())

context['handle'] = unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
