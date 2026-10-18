"""Two-player PIE: jumping disabled for both players; normal movement preserved."""
import unreal
import time
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Office_Pack_Vol_1/Maps/DemoMap')
c={'stage':0,'start':time.monotonic(),'handle':None,'seen':set(),'counts':{},'volumes':{}}
def tick(delta,c=c,u=unreal,t=time):
    try:
        if t.monotonic()-c['start']>140:raise RuntimeError('timeout')
        ws=u.EditorLevelLibrary.get_pie_worlds(True)
        s=next((w for w in ws if u.GameplayStatics.get_game_mode(w)),None)
        cl=next((w for w in ws if not u.GameplayStatics.get_game_mode(w)),None)
        if not s or not cl:return
        hp=u.GameplayStatics.get_player_pawn(s,0);sp=u.GameplayStatics.get_player_pawn(s,1);cp=u.GameplayStatics.get_player_pawn(cl,0)
        if not hp or not sp or not cp or not cp.is_locally_controlled():return
        now=u.GameplayStatics.get_time_seconds(s)
        if c['stage']==0:
            if not c.get('ready'):c['ready']=now;return
            if now-c['ready']<2:return
            ais=u.GameplayStatics.get_all_actors_of_class(s,u.ArenaAIController)
            for ai in ais:
                brain=ai.get_component_by_class(u.BrainComponent)
                if brain:brain.stop_logic('Footstep audio test')
                for sense in [u.AISense_Sight,u.AISense_Hearing,u.AISense_Damage,u.AISense_Team]:ai.get_component_by_class(u.AIPerceptionComponent).set_sense_enabled(sense,False)
            floor=u.CoopPIETestLibrary.spawn_test_actor(s,u.StaticMeshActor,u.Transform(location=u.Vector(20000,20000,-50),scale=u.Vector(40,40,1)))
            mesh=floor.get_component_by_class(u.StaticMeshComponent)
            mesh.set_mobility(u.ComponentMobility.MOVABLE)
            mesh.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
            mesh.set_collision_profile_name('BlockAll')
            mesh.set_is_replicated(True)
            floor.set_replicates(True)
            floor.set_replicate_movement(True)
            guard=next(p for p in u.GameplayStatics.get_all_actors_of_class(s,u.ArenaGameCharacter) if p.get_controller() in ais)
            for i,p in enumerate([hp,sp,guard]):
                p.set_editor_property('can_be_damaged',False)
                p.set_actor_location(u.Vector(20000,20000+i*180,120),False,True)
                p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            cp.set_actor_location(u.Vector(20000,20180,120),False,True)
            c['guard']=guard;c['hostid']=hp.get_editor_property('player_state').get_editor_property('player_id')
            c['clientid']=sp.get_editor_property('player_state').get_editor_property('player_id')
            c['stage']=1;c['at']=now
            return
        dt=now-c['at']
        if c['stage']==1 and dt>1:
            assert not hp.can_jump() and not cp.can_jump(),'jump still allowed'
            c['z']=[hp.get_actor_location().z,cp.get_actor_location().z]
            hp.jump();cp.jump()
            c['stage']=2;c['at']=now
        elif c['stage']==2 and dt>0.4:
            for i,p in enumerate([hp,cp]):
                assert abs(p.get_actor_location().z-c['z'][i])<3,('jump changed height',p.get_actor_location(),c['z'])
                assert abs(p.get_velocity().z)<3,'jump generated vertical velocity'
                p.stop_jumping()
            u.log('NO_JUMP_TEST PASS: host and client cannot jump, including direct Blueprint-callable Jump requests')
            c['positions']=[hp.get_actor_location().x,cp.get_actor_location().x]
            c['stage']=3;c['at']=now
        elif c['stage']==3:
            hp.add_movement_input(u.Vector(1,0,0),1,False)
            cp.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<1:return
            for i,p in enumerate([hp,cp]):assert p.get_actor_location().x-c['positions'][i]>50,'walking blocked'
            u.log('NO_JUMP_TEST PASS: both players still walk normally')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('NO_JUMP_TEST FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
