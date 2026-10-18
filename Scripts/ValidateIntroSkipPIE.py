"""Listen-server intro: client-first vote, host-first vote, natural completion."""
import unreal
import time
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/MainMenu')
ctx = {'stage': 0, 'round': 0, 'start': time.monotonic(), 'at': time.monotonic(), 'handle': None}
def tick(delta, c=ctx, u=unreal, t=time):
    now = t.monotonic()
    try:
        if now-c['start'] > 240: raise RuntimeError('timeout stage='+str(c['stage']))
        worlds = u.EditorLevelLibrary.get_pie_worlds(True)
        server = next((w for w in worlds if u.GameplayStatics.get_game_mode(w)), None)
        client = next((w for w in worlds if not u.GameplayStatics.get_game_mode(w)), None)
        if not server or not client: return
        for w in worlds:
            for p in u.GameplayStatics.get_all_actors_of_class(w, u.ArenaGameCharacter):
                p.set_editor_property('can_be_damaged', False)
        if c['stage'] == 0:
            if 'MainMenu.' not in server.get_path_name() or 'MainMenu.' not in client.get_path_name(): return
            if not all(u.WidgetLibrary.get_all_widgets_of_class(w, u.MainMenuWidget, False) for w in worlds): return
            sp = u.GameplayStatics.get_player_controller(server, 0)
            cp = u.GameplayStatics.get_player_controller(client, 0)
            c['sp'] = sp; c['cp'] = cp
            u.CoopPIETestLibrary.queue_action(cp if c['round'] == 0 else sp, 'Start')
            c['stage'] = 1; c['at'] = now
        elif c['stage'] == 1 and now-c['at'] > 2:
            assert 'MainMenu.' in server.get_path_name()
            assert not u.CoopPIETestLibrary.is_game_input_ready(c['sp'])
            assert not u.CoopPIETestLibrary.is_game_input_ready(c['cp'])
            if c['round'] == 2:
                c['stage'] = 3; c['at'] = now
            else:
                first = c['cp'] if c['round'] == 0 else c['sp']
                u.CoopPIETestLibrary.queue_action(first, 'VoteSkip')
                u.CoopPIETestLibrary.queue_action(first, 'VoteSkip')
                c['stage'] = 2; c['at'] = now
        elif c['stage'] == 2 and now-c['at'] > 1:
            assert 'MainMenu.' in server.get_path_name() and 'MainMenu.' in client.get_path_name(), 'one vote or duplicate vote skipped team intro'
            u.log('INTRO_TEST PASS: one vote and duplicate vote do not skip, round='+str(c['round']))
            u.CoopPIETestLibrary.queue_action(c['sp'] if c['round'] == 0 else c['cp'], 'VoteSkip')
            c['stage'] = 3; c['at'] = now
        elif c['stage'] == 3:
            if 'DemoMap.' not in server.get_path_name() or 'DemoMap.' not in client.get_path_name(): return
            ps = u.GameplayStatics.get_player_pawn(server, 0)
            pc = u.GameplayStatics.get_player_pawn(client, 0)
            if not ps or not pc: return
            for w in worlds:
                ctl = u.GameplayStatics.get_player_controller(w, 0)
                assert u.CoopPIETestLibrary.is_game_input_ready(ctl), 'viewport/UI mode or move/look input still blocked'
                assert not u.WidgetLibrary.get_all_widgets_of_class(w, u.MainMenuWidget, False), 'menu survived travel'
            ps.set_actor_location(u.Vector(-2370,1020,92.15),False,True)
            remote = u.GameplayStatics.get_player_pawn(server,1)
            remote.set_actor_location(u.Vector(-2370,1200,92.15),False,True)
            pc.set_actor_location(u.Vector(-2370,1200,92.15),False,True)
            c['stage'] = 4; c['at'] = now
        elif c['stage'] == 4:
            if now-c['at'] < 0.4: return
            ps = u.GameplayStatics.get_player_pawn(server,0)
            pc = u.GameplayStatics.get_player_pawn(client,0)
            if 'positions' not in c:
                c['positions'] = [ps.get_actor_location(), pc.get_actor_location()]
            for p in [ps,pc]: p.add_movement_input(u.Vector(1,0,0),1,False)
            if now-c['at'] < 1.5: return
            for i,p in enumerate([ps,pc]):
                assert p.get_actor_location().x - c['positions'][i].x > 50, 'player did not move'
            u.log('INTRO_TEST PASS: both viewports accept game input and both players move; round='+str(c['round']))
            c.pop('positions')
            if c['round'] == 2:
                u.unregister_slate_post_tick_callback(c['handle'])
                u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
                c['handle'] = u.register_slate_post_tick_callback(lambda dt, ue=u: ue.SystemLibrary.quit_editor())
            else:
                c['round'] += 1; c['stage'] = 0; c['at'] = now
                u.CoopPIETestLibrary.queue_action(u.GameplayStatics.get_player_controller(server, 0), 'ReturnMenu')
    except Exception as ex:
        u.log_error('INTRO_TEST FAIL '+repr(ex)+' stage='+str(c['stage'])+' round='+str(c['round']))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle'] = u.register_slate_post_tick_callback(lambda dt, ue=u: ue.SystemLibrary.quit_editor())
ctx['handle'] = unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
