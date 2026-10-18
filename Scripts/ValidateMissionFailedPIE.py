"""Use -MissionFailurePlayers=1 or 2. All damage/state changes are PIE-only."""
import unreal as u
import time
import re
match=re.search(r'MissionFailurePlayers=(\d+)',u.SystemLibrary.get_command_line())
expected=int(match.group(1)) if match else 1
u.EditorPythonScripting.set_keep_python_script_alive(True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
c={'stage':0,'start':time.monotonic(),'handle':None,'min_text':1.,'max_text':0.}

def tick(delta,c=c,u=u,t=time,expected=expected):
    try:
        now=t.monotonic()
        if now-c['start']>120:raise RuntimeError('timeout')
        worlds=u.EditorLevelLibrary.get_pie_worlds(True)
        if len(worlds)!=expected:return
        server=next((w for w in worlds if u.GameplayStatics.get_game_mode(w)),None)
        if not server:return
        if c['stage']==0:
            gs=u.GameplayStatics.get_game_state(server)
            players=[u.GameplayStatics.get_player_pawn(server,i) for i in range(expected)]
            if not isinstance(gs,u.ArenaGameState) or not all(players):return
            if gs.get_editor_property('connected_players')!=expected:return
            for p in players:p.set_editor_property('can_be_damaged',False)
            for ai in u.GameplayStatics.get_all_actors_of_class(server,u.ArenaAIController):
                brain=ai.get_component_by_class(u.BrainComponent)
                if brain:brain.stop_logic('Mission Failed test')
            u.CoopPIETestLibrary.queue_action(players[-1],'AIHit')
            c.update(stage=1 if expected==2 else 2,at=now)
            return
        if c['stage']==1:
            gs=u.GameplayStatics.get_game_state(server)
            assert not gs.get_editor_property('team_failed'),'one death failed a multiplayer round'
            for w in worlds:assert not u.WidgetLibrary.get_all_widgets_of_class(w,u.MissionFailedWidget,False)
            if now-c['at']<1:return
            assert len(gs.get_editor_property('dead_players'))==1
            u.log('MISSION_FAILED PASS: one dead teammate spectates, no failure screen while another lives')
            u.CoopPIETestLibrary.queue_action(u.GameplayStatics.get_player_pawn(server,0),'AIHit')
            c.update(stage=2,at=now)
            return
        if all('MainMenu.' in w.get_path_name() for w in worlds):
            assert c.get('shown'),'failure screen was not shown'
            assert c['min_text']<.5 and c['max_text']>.99,('text did not fade in',c['min_text'],c['max_text'])
            assert now-c['at']>=4.8,'returned before failure presentation'
            if any(not u.WidgetLibrary.get_all_widgets_of_class(w,u.MainMenuWidget,False) for w in worlds):return
            u.log('MISSION_FAILED ALL PASSED players='+str(expected)+': text fades in, black overlay, then all return to Menu')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
            return
        gs=u.GameplayStatics.get_game_state(server)
        if not isinstance(gs,u.ArenaGameState) or not gs.get_editor_property('team_failed'):return
        assert len(gs.get_editor_property('dead_players'))==expected,'incorrect all-dead count'
        for w in worlds:
            failed=u.WidgetLibrary.get_all_widgets_of_class(w,u.MissionFailedWidget,False)
            if not failed:return
            assert len(failed)==1,('duplicate failure widgets',len(failed))
            text=next(text for text in u.ObjectIterator(u.TextBlock)
                if text.get_name()=='MissionFailedText' and text.get_outer().get_outer()==failed[0])
            assert str(text.get_text())=='MISSION FAILED','incorrect failure label'
            alpha=text.get_render_opacity()
            c['min_text']=min(c['min_text'],alpha);c['max_text']=max(c['max_text'],alpha)
            if now-c['at']>2.3:
                assert failed[0].get_render_opacity()>.99,'background did not finish fading'
                for a in u.GameplayStatics.get_all_actors_of_class(w,u.AmbientSound):
                    if 'OfficeBGM' in a.tags:assert not a.get_component_by_class(u.AudioComponent).is_playing(),'BGM still playing after failure'
        c['shown']=True
    except Exception as ex:
        import traceback
        u.log_error(traceback.format_exc())
        u.log_error('MISSION_FAILED FAIL players='+str(expected)+' stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
