"""PIE controller regression using controlled sight events and transient actors."""
import unreal
import time
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
ctx={'stage':0,'start':time.monotonic(),'handle':None}
def tick(delta,c=ctx,u=unreal,t=time):
    try:
        if t.monotonic()-c['start']>200:raise RuntimeError('timeout')
        worlds=u.EditorLevelLibrary.get_pie_worlds(True)
        w=next((x for x in worlds if u.GameplayStatics.get_game_mode(x)),None)
        cw=next((x for x in worlds if not u.GameplayStatics.get_game_mode(x)),None)
        if not w or not cw:return
        p=[u.GameplayStatics.get_player_pawn(w,i) for i in range(2)]
        cp=u.GameplayStatics.get_player_pawn(cw,0)
        if not all(p) or not cp or not cp.is_locally_controlled():return
        now=u.GameplayStatics.get_time_seconds(w)
        def place(index,pos):
            p[index].set_actor_location(pos,False,True)
            if index==1:cp.set_actor_location(pos,False,True)
        def wall(y=1020,width=8):
            tr=u.Transform(location=u.Vector(-2120,y,140),scale=u.Vector(0.2,width,4))
            a=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,tr)
            mesh=a.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE)
            mesh.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
            mesh.set_collision_profile_name('BlockAll')
            return a
        if c['stage']==0:
            if not c.get('ready'):c['ready']=now;return
            if now-c['ready']<2:return
            ais=u.GameplayStatics.get_all_actors_of_class(w,u.ArenaAIController)
            assert ais,'no guards'
            ai=ais[0]
            for actor in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter)+[cp]:
                actor.set_editor_property('can_be_damaged',False)
                actor.get_component_by_class(u.CharacterMovementComponent).disable_movement()
            for other in ais:
                sense=other.get_component_by_class(u.AIPerceptionComponent)
                sense.set_sense_enabled(u.AISense_Hearing,False)
                sense.set_sense_enabled(u.AISense_Damage,False)
                sense.set_sense_enabled(u.AISense_Team,False)
                sense.set_sense_enabled(u.AISense_Sight,False)
            pawn=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter) if a.get_controller()==ai)
            place(0,u.Vector(-7000,-7000,92.15));place(1,u.Vector(-7000,-6500,92.15))
            pawn.set_actor_location(u.Vector(-2370,1020,92.15),False,True)
            pawn.set_actor_rotation(u.Rotator(pitch=0,yaw=0,roll=0),True)
            ai.set_control_rotation(u.Rotator(pitch=0,yaw=0,roll=0))
            for target in p:u.CoopPIETestLibrary.queue_sight_event(ai,target,False)
            c['pawn']=pawn
            c['ai']=ai;c['bb']=ai.get_component_by_class(u.BlackboardComponent)
            c['stage']=11;c['at']=now
            return
        ai=c['ai'];bb=c['bb'];dt=now-c['at']
        if c['stage']==11 and dt>7:
            place(0,u.Vector(-1870,1020,92.15))
            u.CoopPIETestLibrary.queue_sight_event(ai,p[0],True)
            c['stage']=1;c['at']=now
        elif c['stage']==1 and dt>0.8:
            progress=ai.get_alert_progress()
            assert 0<progress<1,('partial sight',progress)
            c['wall']=wall();u.CoopPIETestLibrary.queue_sight_event(ai,p[0],False);c['stage']=2;c['at']=now
        elif c['stage']==2 and dt>3:
            assert ai.get_alert_progress()==0,('partial decay',ai.get_alert_progress())
            assert not bb.get_value_as_object('TargetActor')
            u.log('AI_TEST PASS: partial detection decays to zero after sight-loss notification')
            c['wall'].destroy_actor();u.CoopPIETestLibrary.queue_sight_event(ai,p[0],True);c['stage']=3;c['at']=now
        elif c['stage']==3 and dt>3.6:
            assert ai.get_alert_progress()>=0.99
            assert bb.get_value_as_object('TargetActor')==p[0]
            c['last_seen']=p[0].get_actor_location()
            c['wall']=wall();u.CoopPIETestLibrary.queue_sight_event(ai,p[0],False);c['stage']=4;c['at']=now
        elif c['stage']==4 and dt>3.3:
            assert not bb.get_value_as_object('TargetActor'),'guard tracks actor through wall'
            assert not bb.get_value_as_bool('AlertConfirmed')
            assert 0<ai.get_alert_progress()<1,ai.get_alert_progress()
            assert bb.is_vector_value_set('InvestigateLocation')
            c['investigate']=bb.get_value_as_vector('InvestigateLocation')
            place(0,u.Vector(-1870,1220,92.15))
            c['stage']=5;c['at']=now
        elif c['stage']==5 and dt>0.8:
            loc=bb.get_value_as_vector('InvestigateLocation')
            assert abs(loc.y-c['investigate'].y)<1,'last-known location followed hidden player'
            u.log('AI_TEST PASS: confirmed target clears after three-second chase grace; investigation uses frozen last-known location')
            c['stage']=6
        elif c['stage']==6 and dt>7:
            assert ai.get_alert_progress()==0
            assert not bb.get_value_as_object('TargetActor')
            assert not bb.is_vector_value_set('InvestigateLocation')
            assert not bb.is_vector_value_set('LastDamageLocation')
            u.log('AI_TEST PASS: confirmed detection and search expire after hiding')
            c['wall'].destroy_actor();place(0,u.Vector(-1870,1020,92.15))
            u.CoopPIETestLibrary.queue_sight_event(ai,p[0],True)
            c['stage']=7;c['at']=now
        elif c['stage']==7 and dt>3.6:
            assert bb.get_value_as_object('TargetActor')==p[0],'guard did not reacquire'
            place(0,u.Vector(-370,1020,92.15))
            u.CoopPIETestLibrary.queue_sight_event(ai,p[0],False)
            c['stage']=8;c['at']=now
        elif c['stage']==8 and dt>10:
            assert ai.get_alert_progress()==0
            assert not bb.get_value_as_object('TargetActor')
            u.log('AI_TEST PASS: sight-loss notification outside range drops pursuit and detection')
            place(0,u.Vector(-1870,920,92.15));place(1,u.Vector(-1870,1120,92.15))
            for target in p:u.CoopPIETestLibrary.queue_sight_event(ai,target,True)
            c['stage']=9;c['at']=now
        elif c['stage']==9 and dt>3.6:
            target=bb.get_value_as_object('TargetActor')
            assert target in p
            index=0 if target==p[0] else 1
            c['other']=p[1-index]
            c['wall']=wall(970 if index==0 else 1070,0.3)
            u.CoopPIETestLibrary.queue_sight_event(ai,target,False)
            c['stage']=10;c['at']=now
        elif c['stage']==10 and dt>1:
            assert bb.get_value_as_object('TargetActor')==c['other'],'lost player prevented switching to visible teammate'
            u.log('AI_TEST PASS: losing one co-op player keeps chasing the other visible player')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('AI_TEST FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
ctx['handle']=unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
