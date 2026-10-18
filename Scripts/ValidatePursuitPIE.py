"""Exercise the real behavior tree: failed paths, resumed movement, sight-memory expiry."""
import unreal as u
import time
u.EditorPythonScripting.set_keep_python_script_alive(True)
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
c = {'stage': 0, 'start': time.monotonic(), 'handle': None}

def tick(delta, c=c, u=u, t=time):
    try:
        if t.monotonic()-c['start'] > 120: raise RuntimeError('timeout')
        worlds = u.EditorLevelLibrary.get_pie_worlds(True)
        w = next((x for x in worlds if u.GameplayStatics.get_game_mode(x)), None)
        if not w: return
        p = u.GameplayStatics.get_player_pawn(w, 0)
        if not p: return
        now = u.GameplayStatics.get_time_seconds(w)
        if c['stage'] == 0:
            if now < 2: return
            ais = u.GameplayStatics.get_all_actors_of_class(w, u.ArenaAIController)
            assert ais
            ai = ais[0]
            guard = next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter) if a.get_controller()==ai)
            original = guard.get_actor_location()
            for controller in ais:
                sense = controller.get_component_by_class(u.AIPerceptionComponent)
                for cls in [u.AISense_Sight,u.AISense_Hearing,u.AISense_Damage,u.AISense_Team]:
                    sense.set_sense_enabled(cls,False)
                controller.get_component_by_class(u.BrainComponent).stop_logic('Isolate pursuit')
            for actor in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):
                actor.set_editor_property('can_be_damaged',False)
                actor.set_editor_property('melee_range',0.0)
            floor=u.CoopPIETestLibrary.spawn_test_actor(w,u.StaticMeshActor,u.Transform(location=u.Vector(20000,20000,-50),scale=u.Vector(40,40,1)))
            mesh=floor.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE)
            mesh.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
            mesh.set_collision_profile_name('BlockAll')
            ai.stop_movement()
            guard.set_actor_location(u.Vector(20000,20000,96.15),False,True)
            p.set_actor_location(u.Vector(20350,20000,96.15),False,True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(ai=ai,guard=guard,original=original,bb=ai.get_component_by_class(u.BlackboardComponent),at=now,stage=1)
            return
        ai=c['ai']; guard=c['guard']; bb=c['bb']; dt=now-c['at']
        if c['stage']==1 and dt>8:
            ai.stop_movement()
            guard.set_actor_location(u.Vector(20000,20000,96.15),False,True)
            guard.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            u.CoopPIETestLibrary.queue_sight_event(ai,p,True)
            ai.get_component_by_class(u.BrainComponent).restart_logic()
            c.update(stage=2,at=now)
        elif c['stage']==2 and dt>6:
            assert bb.get_value_as_object('TargetActor')==p,'failed path lost target'
            assert ai.get_alert_progress()==1,'failed path reset alert'
            assert ai.get_focus_actor()==p,'guard is not watching unreachable player'
            assert (guard.get_actor_location()-u.Vector(20000,20000,96.15)).length()<50,('guard wandered after path failure',guard.get_actor_location())
            u.log('PURSUIT PASS: real behavior tree retains chase/focus after repeated unreachable path failures')
            # Return to baked navigation and select a reachable point far enough
            # away to verify movement, instead of merely checking request success.
            target=None
            navigation=u.NavigationSystemV1.get_navigation_system(w)
            assert navigation
            for attempt in range(60):
                point=navigation.get_random_reachable_point_in_radius(w,c['original'],800)
                if point and (point-c['original']).length()>400:
                    target=point+u.Vector(0,0,96.15);break
            assert target,'no reachable test point on DemoMap navigation'
            ai.stop_movement()
            guard.set_actor_location(c['original'],False,True)
            p.set_actor_location(target,False,True)
            c.update(stage=3,at=now,move_start=guard.get_actor_location())
        elif c['stage']==3:
            moved=(guard.get_actor_location()-c['move_start']).length()
            if moved<=80:
                assert dt<5,'guard did not resume navigation'
                return
            u.log('PURSUIT PASS: guard resumes moving when the player becomes reachable')
            p.set_actor_location(u.Vector(21000,20000,96.15),False,True)
            c.update(stage=4,at=now)
        elif c['stage']==4 and dt>2:
            path=u.AIHelperLibrary.get_current_path(ai)
            assert path and path.is_partial(),'guard did not accept a partial path to off-nav player'
            assert bb.get_value_as_object('TargetActor')==p
            u.log('PURSUIT PASS: off-nav player produces a partial path toward reachable boundary')
            u.CoopPIETestLibrary.queue_sight_event(ai,p,False)
            c.update(stage=5,at=now)
        elif c['stage']==5 and dt>1:
            assert bb.get_value_as_object('TargetActor')==p,'three-second memory ended early'
            c['stage']=6
        elif c['stage']==6 and dt>3.5:
            assert not bb.get_value_as_object('TargetActor'),'unreachable chase did not release after memory expired'
            assert ai.get_focus_actor()!=p,'focus leaked after chase abort'
            assert ai.get_alert_progress()<1,'alert stayed full after loss'
            u.log('PURSUIT PASS: normal three-second chase grace expires and releases focus')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('PURSUIT FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=u.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
