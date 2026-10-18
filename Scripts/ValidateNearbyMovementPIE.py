"""Real 360-degree proximity checks: movement, crouch exclusion, walls, and expiry."""
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
            guard.set_actor_rotation(u.Rotator(0,0,0),True)
            ai.set_control_rotation(u.Rotator(0,0,0))
            for actor in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):actor.set_editor_property('can_be_damaged',False)
            p.set_actor_location(u.Vector(19850,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(ai=ai,guard=guard,bb=ai.get_component_by_class(u.BlackboardComponent),at=now,stage=1)
            return
        ai=c['ai'];bb=c['bb'];dt=now-c['at']
        def place(distance):
            p.set_actor_location(u.Vector(20000-distance,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            p.consume_movement_input_vector()
        if c['stage']==1 and dt>7:
            assert ai.get_alert_progress()==0,'stationary unknown player detected'
            p.crouch();c.update(stage=2,at=now)
        elif c['stage']==2:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert p.get_editor_property('is_crouched')
            assert ai.get_alert_progress()==0,'sneaking caused close movement detection'
            u.log('NEARBY_AI PASS: stationary and crouching players do not trigger new proximity alert')
            p.un_crouch();place(150)
            wall=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,u.Transform(location=u.Vector(19925,20000,150),scale=u.Vector(.1,8,4)))
            mesh=wall.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE);mesh.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'));mesh.set_collision_profile_name('BlockAll')
            c['wall']=mesh;c.update(stage=3,at=now)
        elif c['stage']==3:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert p.get_velocity().length()>20
            assert ai.get_alert_progress()==0,'detected through a wall'
            u.log('NEARBY_AI PASS: wall blocks detection inside 2m')
            c['wall'].set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
            place(250);c.update(stage=4,at=now)
        elif c['stage']==4:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.3:return
            assert ai.get_alert_progress()==0,'walking outside 2m detected behind guard'
            u.log('NEARBY_AI PASS: walking behind guard outside 2m does not detect')
            place(150);c.update(stage=5,at=now)
        elif c['stage']==5:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.2:return
            assert ai.get_alert_progress()==1
            assert bb.get_value_as_object('TargetActor')==p
            u.log('NEARBY_AI PASS: walking behind guard inside 2m instantly selects chase target')
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            p.consume_movement_input_vector();c.update(stage=6,at=now)
        elif c['stage']==6 and dt>.3:
            assert bb.get_value_as_object('TargetActor')==p,'known close player disappeared by stopping'
            place(500);c.update(stage=7,at=now)
        elif c['stage']==7 and dt>3.3:
            assert not bb.get_value_as_object('TargetActor'),'target kept tracking outside proximity without sight'
            assert bb.is_vector_value_set('InvestigateLocation')
            c.update(stage=8,at=now)
        elif c['stage']==8 and dt>7:
            assert ai.get_alert_progress()==0
            assert not bb.is_vector_value_set('InvestigateLocation')
            u.log('NEARBY_AI PASS: leaving radius releases actor, searches last location, then expires')
            p.call_method('ServerSetSprintState',args=(True,))
            place(150);c.update(stage=9,at=now)
        elif c['stage']==9:
            p.add_movement_input(u.Vector(0,1,0),1,False)
            if dt<.2:return
            assert ai.get_alert_progress()==1
            assert bb.get_value_as_object('TargetActor')==p
            u.log('NEARBY_AI ALL PASSED: sprinting behind guard inside 2m also confirms immediately')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('NEARBY_AI FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
