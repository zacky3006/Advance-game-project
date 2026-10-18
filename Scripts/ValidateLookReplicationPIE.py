"""Regression: stationary first-person mouse yaw on client and listen host."""
import unreal
import time
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
ctx={'stage':0,'start':time.monotonic(),'at':time.monotonic(),'handle':None}
def tick(delta,c=ctx,u=unreal,t=time):
    try:
        now=t.monotonic()
        if now-c['start'] > 100: raise RuntimeError('timeout')
        worlds=u.EditorLevelLibrary.get_pie_worlds(True)
        s=next((w for w in worlds if u.GameplayStatics.get_game_mode(w)),None)
        cl=next((w for w in worlds if not u.GameplayStatics.get_game_mode(w)),None)
        if not s or not cl:return
        host=u.GameplayStatics.get_player_pawn(s,0)
        remote=u.GameplayStatics.get_player_pawn(s,1)
        client=u.GameplayStatics.get_player_pawn(cl,0)
        if not host or not remote or not client:return
        for p in [host,remote,client]:p.set_editor_property('can_be_damaged',False)
        hp=u.GameplayStatics.get_player_controller(s,0)
        cp=u.GameplayStatics.get_player_controller(cl,0)
        if c['stage']==0:
            if not client.is_locally_controlled() or not client.is_player_controlled(): return
            if not c.get('ready_at'): c['ready_at']=now; return
            if now-c['ready_at']<2: return
            u.log('LOOK_TEST initial: client yaw='+str(client.get_editor_property('use_controller_rotation_yaw'))+' first='+str(client.get_editor_property('is_first_person'))+' local='+str(client.is_locally_controlled()))
            host.set_actor_location(u.Vector(-2370,1020,92.15),False,True)
            remote.set_actor_location(u.Vector(-2370,1200,92.15),False,True)
            client.set_actor_location(u.Vector(-2370,1200,92.15),False,True)
            u.CoopPIETestLibrary.queue_action(client,'TogglePerspective')
            c['stage']=1;c['at']=now
        elif now-c['at']>1:
            if c['stage']==1:
                assert client.get_editor_property('use_controller_rotation_yaw'), 'local toggle did not run; first='+str(client.get_editor_property('is_first_person'))
                assert remote.get_editor_property('use_controller_rotation_yaw'), 'server did not receive first-person rotation policy'
                cp.set_control_rotation(u.Rotator(pitch=0,yaw=60,roll=0))
            elif c['stage']==2:
                assert abs(remote.get_actor_rotation().yaw-60)<3, remote.get_actor_rotation()
                cp.set_control_rotation(u.Rotator(pitch=0,yaw=-60,roll=0))
            elif c['stage']==3:
                assert abs(remote.get_actor_rotation().yaw+60)<3, remote.get_actor_rotation()
                u.log('LOOK_TEST PASS: server sees stationary client yaw swing +60 to -60 in first person')
                u.CoopPIETestLibrary.queue_action(host,'TogglePerspective')
                hp.set_control_rotation(u.Rotator(pitch=0,yaw=60,roll=0))
            elif c['stage']==4:
                other=next(p for p in u.GameplayStatics.get_all_actors_of_class(cl,u.ArenaGameCharacter) if p != client and p.get_class()==client.get_class())
                assert abs(other.get_actor_rotation().yaw-60)<3,other.get_actor_rotation()
                hp.set_control_rotation(u.Rotator(pitch=0,yaw=-60,roll=0))
            elif c['stage']==5:
                other=next(p for p in u.GameplayStatics.get_all_actors_of_class(cl,u.ArenaGameCharacter) if p != client and p.get_class()==client.get_class())
                assert abs(other.get_actor_rotation().yaw+60)<3,other.get_actor_rotation()
                u.log('LOOK_TEST PASS: client sees stationary host yaw swing +60 to -60')
                u.CoopPIETestLibrary.queue_action(client,'TogglePerspective')
                u.CoopPIETestLibrary.queue_action(host,'TogglePerspective')
            elif c['stage']==6:
                for p in [host,remote,client]:
                    assert not p.get_editor_property('use_controller_rotation_yaw'), 'third-person rotation policy not restored'
                    assert p.get_component_by_class(u.CharacterMovementComponent).get_editor_property('orient_rotation_to_movement')
                u.log('LOOK_TEST PASS: returning to third person restores movement-facing rotation on owner and server')
                u.unregister_slate_post_tick_callback(c['handle'])
                u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
                c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
                return
            c['stage']+=1;c['at']=now
    except Exception as ex:
        u.log_error('LOOK_TEST FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
ctx['handle']=unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
