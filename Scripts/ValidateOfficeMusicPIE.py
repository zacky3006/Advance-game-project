"""PIE: BGM starts once, loops past song end, then fades when the round ends."""
import unreal as u
import time
u.EditorPythonScripting.set_keep_python_script_alive(True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
c={'stage':0,'start':time.monotonic(),'handle':None}

def tick(delta,c=c,u=u,t=time):
    try:
        if t.monotonic()-c['start']>100:raise RuntimeError('timeout')
        worlds=u.EditorLevelLibrary.get_pie_worlds(True)
        if not worlds:return
        server=next((w for w in worlds if u.GameplayStatics.get_game_mode(w)),None)
        if not server:return
        now=u.GameplayStatics.get_time_seconds(server)
        if c['stage']==0:
            if now<3:return
            c['audio']=[]
            for w in worlds:
                for p in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):p.set_editor_property('can_be_damaged',False)
                found=[a for a in u.GameplayStatics.get_all_actors_of_class(w,u.AmbientSound) if 'OfficeBGM' in a.tags]
                assert len(found)==1,(w.get_name(),len(found))
                assert not any('OfficeHVAC' in a.tags for a in u.GameplayStatics.get_all_actors_of_class(w,u.AmbientSound))
                audio=found[0].get_component_by_class(u.AudioComponent)
                assert audio.is_playing(),'BGM not started'
                sound=audio.get_editor_property('sound')
                assert sound.get_editor_property('looping')
                length=sound.get_editor_property('duration')
                assert length>200,length
                audio.stop();audio.play(length-.25)
                c['audio'].append(audio)
            u.log('OFFICE_MUSIC PASS: one active looping song per world, HVAC replaced')
            c['stage']=1;c['at']=now
        elif c['stage']==1 and now-c['at']>1.5:
            assert all(a.is_playing() for a in c['audio']),'BGM stopped at song end'
            u.log('OFFICE_MUSIC PASS: playback continues past end by looping')
            team=u.GameplayStatics.get_game_state(server)
            team.set_editor_property('bTeamComplete',True)
            team.force_net_update()
            c['stage']=2;c['at']=now
        elif c['stage']==2 and now-c['at']>2:
            assert all(not a.is_playing() for a in c['audio']),'BGM continued after round completion'
            u.log('OFFICE_MUSIC ALL PASSED: round completion fades and stops BGM')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('OFFICE_MUSIC FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
