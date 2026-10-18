"""Real sight-interface collision checks for head exposure, crouching and office sofa."""
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
        def check():
            result=u.CoopPIETestLibrary.check_player_visibility(p,c['guard'])
            # Unreal can expose bool + one output as output-or-None.
            if isinstance(result,tuple):return result
            return result is not None,result if result is not None else u.Vector()
        if c['stage']==1 and dt>2:
            place(800)
            def obstacle(asset,position,scale):
                actor=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,u.Transform(location=position,scale=scale))
                mesh=actor.get_component_by_class(u.StaticMeshComponent)
                mesh.set_mobility(u.ComponentMobility.MOVABLE);mesh.set_static_mesh(u.load_asset(asset));mesh.set_collision_profile_name('BlockAll')
                return actor,mesh
            actor,mesh=obstacle('/Engine/BasicShapes/Cube',u.Vector(20700,20000,65),u.Vector(.2,8,1.3))
            c['obstacle']=actor;c['mesh']=mesh;c.update(stage=2,at=now)
        elif c['stage']==2 and dt>.5:
            eye,rotation=c['guard'].get_actor_eyes_view_point()
            center_hit=u.SystemLibrary.line_trace_single(w,eye,p.get_actor_location(),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[p,c['guard']],u.DrawDebugTrace.NONE,True)
            assert center_hit is not None,'fixture does not block the old center-only sight ray'
            visible,seen=check()
            assert visible,'standing head hidden by low cover'
            assert seen.z>p.get_actor_location().z,'only center visibility used'
            u.log('LOW_COVER PASS: standing head visible over low cover, seen='+str(seen))
            p.crouch();c.update(stage=3,at=now)
        elif c['stage']==3 and dt>.5:
            assert p.get_editor_property('is_crouched')
            visible,seen=check()
            assert not visible,'crouched player seen through low cover'
            u.log('LOW_COVER PASS: crouching behind same cover blocks all body samples')
            p.un_crouch();c['mesh'].set_world_scale3d(u.Vector(.2,8,3.0))
            c['obstacle'].set_actor_location(u.Vector(20700,20000,150),False,True)
            c.update(stage=4,at=now)
        elif c['stage']==4 and dt>.5:
            visible,seen=check()
            assert not visible,'standing player seen through full-height wall'
            u.log('LOW_COVER PASS: full-height wall blocks standing player')
            c['mesh'].set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
            c.update(stage=5,at=now)
        elif c['stage']==5 and dt>.3:
            assert check()[0]
            c['obstacle'].destroy_actor()
            actor=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,u.Transform(location=u.Vector(20700,20000,0)))
            mesh=actor.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE)
            mesh.set_static_mesh(u.load_asset('/Game/Office_Pack_Vol_1/Models/SM_Sofa_2'))
            mesh.set_collision_profile_name('BlockAll')
            origin,extent=actor.get_actor_bounds(False)
            u.log('LOW_COVER SOFA actual_top='+str(origin.z+extent.z)+' extent='+str(extent))
            c['sofa']=actor;c.update(stage=6,at=now)
        elif c['stage']==6 and dt>.5:
            visible,seen=check()
            assert visible,'standing head blocked by office sofa'
            u.log('LOW_COVER ALL PASSED: actual Office Pack sofa permits exposed standing head')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('LOW_COVER FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
