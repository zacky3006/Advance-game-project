"""Controlled sight events plus actual movement: sprint instant detection at 7m."""
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
            u.CoopPIETestLibrary.queue_sight_event(ai,p,True)
            c.update(stage=2,at=now)
        elif c['stage']==2:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert 0<ai.get_alert_progress()<1
            assert not bb.get_value_as_object('TargetActor')
            u.log('SPRINT_AI PASS: normal movement at 6.5m fills gradually')
            place(650);p.call_method('ServerSetSprintState',args=(True,))
            c.update(stage=3,at=now)
        elif c['stage']==3 and dt>.3:
            assert p.get_velocity().length()<20,'fixture still moving'
            assert ai.get_alert_progress()<1,'stationary sprint detected instantly'
            u.log('SPRINT_AI PASS: holding sprint while stationary does not instantly confirm')
            place(750);c.update(stage=4,at=now)
        elif c['stage']==4:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert p.get_velocity().length()>20
            assert ai.get_alert_progress()<1,'sprint outside 7m instantly confirmed'
            assert not bb.get_value_as_object('TargetActor')
            u.log('SPRINT_AI PASS: moving sprint outside 7m still fills gradually')
            place(650);c.update(stage=5,at=now)
        elif c['stage']==5:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.2:return
            assert p.get_velocity().length()>20
            assert ai.get_alert_progress()==1,ai.get_alert_progress()
            assert bb.get_value_as_object('TargetActor')==p
            u.log('SPRINT_AI PASS: moving sprint inside 7m instantly confirms and selects chase target')
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            c.update(stage=6,at=now)
        elif c['stage']==6 and dt>10:
            assert ai.get_alert_progress()==0
            assert not bb.get_value_as_object('TargetActor')
            place(650);c.update(stage=7,at=now)
        elif c['stage']==7:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert ai.get_alert_progress()==0
            assert not bb.get_value_as_object('TargetActor')
            u.log('SPRINT_AI ALL PASSED: sight loss expires chase; unseen sprint does not force detection')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('SPRINT_AI FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
