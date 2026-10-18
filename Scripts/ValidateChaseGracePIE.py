"""Confirmed pursuit persists for three seconds after sight loss, then searches."""
import unreal as u
import time
u.EditorPythonScripting.set_keep_python_script_alive(True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
c={'stage':0,'start':time.monotonic(),'handle':None}

def tick(delta,c=c,u=u,t=time):
    try:
        if t.monotonic()-c['start']>120:raise RuntimeError('timeout')
        worlds=u.EditorLevelLibrary.get_pie_worlds(True)
        w=next((x for x in worlds if u.GameplayStatics.get_game_mode(x)),None)
        if not w:return
        p=u.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        now=u.GameplayStatics.get_time_seconds(w)
        if c['stage']==0:
            if now<2:return
            ais=u.GameplayStatics.get_all_actors_of_class(w,u.ArenaAIController)
            assert ais
            for ai in ais:
                brain=ai.get_component_by_class(u.BrainComponent)
                if brain:brain.stop_logic('Sprint detection test')
                sense=ai.get_component_by_class(u.AIPerceptionComponent)
                for cls in [u.AISense_Sight,u.AISense_Hearing,u.AISense_Damage,u.AISense_Team]:sense.set_sense_enabled(cls,False)
            ai=ais[0]
            guard=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter) if a.get_controller()==ai)
            floor=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,u.Transform(location=u.Vector(20000,20000,-50),scale=u.Vector(40,40,1)))
            mesh=floor.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE);mesh.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'));mesh.set_collision_profile_name('BlockAll')
            guard.set_actor_location(u.Vector(20000,20000,96.15),False,True)
            guard.get_component_by_class(u.CharacterMovementComponent).disable_movement()
            for actor in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):actor.set_editor_property('can_be_damaged',False)
            p.set_actor_location(u.Vector(20650,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(ai=ai,guard=guard,bb=ai.get_component_by_class(u.BlackboardComponent),at=now,stage=1)
            return
        ai=c['ai'];bb=c['bb'];dt=now-c['at']
        def place(distance):
            p.set_actor_location(u.Vector(20000+distance,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            p.consume_movement_input_vector()
        if c['stage']==1 and dt>7:
            place(350)
            u.CoopPIETestLibrary.queue_sight_event(ai,p,True)
            c.update(stage=2,at=now)
        elif c['stage']==2 and dt>.3:
            assert ai.get_alert_progress()==1
            assert bb.get_value_as_object('TargetActor')==p
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            place(800);p.call_method('ServerSetSprintState',args=(True,))
            c.update(stage=3,at=now)
        elif c['stage']==3:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.6:return
            assert p.get_velocity().length()>20
            assert bb.get_value_as_object('TargetActor')==p
            assert ai.get_alert_progress()==1
            u.log('CHASE_GRACE PASS: confirmed player still chased while sprinting out of sight')
            p.call_method('ServerSetSneakState',args=(True,))
            c['stage']=4
        elif c['stage']==4:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<2.5:return
            assert p.get_editor_property('is_crouched')
            assert bb.get_value_as_object('TargetActor')==p
            assert ai.get_alert_progress()==1
            u.log('CHASE_GRACE PASS: crouching out of sight does not break the three-second chase')
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            p.consume_movement_input_vector();c['stage']=5
        elif c['stage']==5 and dt>3.3:
            assert not bb.get_value_as_object('TargetActor'),'chase exceeded grace'
            assert 0<ai.get_alert_progress()<1
            assert bb.is_vector_value_set('InvestigateLocation')
            c['search']=bb.get_value_as_vector('InvestigateLocation')
            place(1500);c.update(stage=6,at=now)
        elif c['stage']==6 and dt>.6:
            assert (bb.get_value_as_vector('InvestigateLocation')-c['search']).length()<1
            assert not bb.get_value_as_object('TargetActor')
            u.log('CHASE_GRACE PASS: at three seconds actor pursuit ends; search location stops following hidden player')
            c['stage']=7
        elif c['stage']==7 and dt>7:
            assert ai.get_alert_progress()==0
            assert not bb.is_vector_value_set('InvestigateLocation')
            p.call_method('ServerSetSneakState',args=(False,))
            place(800);u.CoopPIETestLibrary.queue_sight_event(ai,p,True)
            c.update(stage=8,at=now)
        elif c['stage']==8 and dt>.3:
            assert 0<ai.get_alert_progress()<1
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(stage=9,at=now)
        elif c['stage']==9 and dt>.4:
            assert not bb.get_value_as_object('TargetActor')
            u.log('CHASE_GRACE ALL PASSED: search expires normally; partial detection never grants chase grace')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('CHASE_GRACE FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
