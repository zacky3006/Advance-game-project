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
            guard.set_actor_rotation(u.Rotator(0,0,0),True)
            ai.set_control_rotation(u.Rotator(0,0,0))
            ai.get_component_by_class(u.AIPerceptionComponent).set_sense_enabled(u.AISense_Hearing,True)
            for actor in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):actor.set_editor_property('can_be_damaged',False)
            p.set_actor_location(u.Vector(20000,20600,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(ai=ai,guard=guard,bb=ai.get_component_by_class(u.BlackboardComponent),at=now,stage=1)
            return
        ai=c['ai'];bb=c['bb'];dt=now-c['at']
        def place(distance):
            p.set_actor_location(u.Vector(20000+distance,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            p.consume_movement_input_vector()
        if c['stage']==1 and dt>10:
            p.call_method('ServerSetSprintState',args=(True,))
            c.update(stage=2,at=now)
        elif c['stage']==2:
            p.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<1:return
            heard=bb.is_vector_value_set('InvestigateLocation')
            assert heard,'real sprint footsteps did not reach hearing callback'
            focal=ai.get_focal_point()
            assert (focal-bb.get_value_as_vector('InvestigateLocation')).length()<1,'not facing actual sound location'
            direction=p.get_actor_location()-c['guard'].get_actor_location()
            forward=c['guard'].get_actor_forward_vector()
            alignment=(forward.x*direction.x+forward.y*direction.y+forward.z*direction.z)/max(direction.length(),1.)
            assert alignment>.8,('guard did not turn toward real footsteps',alignment)
            u.log('HEARING_REACTION PASS: real sprint noise sets investigation and turns stationary guard toward sound')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('HEARING_REACTION FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
