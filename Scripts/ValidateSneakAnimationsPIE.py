"""Two-player PIE: replicated crouch blends, actual head lowering and walk cycle."""
import unreal
import time
registry=unreal.AssetRegistryHelpers.get_asset_registry()
options=unreal.AssetRegistryDependencyOptions(True,True,True,True,True)
for folder in ['/Game/Characters/Mannequins/Anims/Sneak','/Game/Characters/Ninja/Sneak','/Game/Characters/John/Sneak']:
    for path in unreal.EditorAssetLibrary.list_assets(folder,recursive=True,include_folder=False):
        for dependency in registry.get_dependencies(path.split('.')[0],options):
            assert not str(dependency).startswith('/MoverExamples/'),(path,str(dependency))
unreal.log('SNEAK_TEST PASS: runtime assets independent of MoverExamples')
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
                if brain:brain.stop_logic('Sneak animation test')
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
            c['meshes']=[];c['heads']=[]
            for w in [s,cl]:
                for p in u.GameplayStatics.get_all_actors_of_class(w,u.ArenaGameCharacter):
                    if not p.get_editor_property('player_state'):continue
                    mesh=p.get_component_by_class(u.SkeletalMeshComponent)
                    mesh.set_editor_property('visibility_based_anim_tick_option',u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
                    inst=mesh.get_anim_instance()
                    assert isinstance(inst,u.SneakAnimInstance),str(inst)
                    head='Head' if mesh.does_socket_exist('Head') else 'head'
                    foot='LeftFoot' if mesh.does_socket_exist('LeftFoot') else 'foot_l'
                    c['meshes'].append((p,mesh,head,foot))
                    c['heads'].append(mesh.get_socket_location(head).z-mesh.get_socket_location(foot).z)
            assert len(c['meshes'])==4,len(c['meshes'])
            hp.crouch();cp.crouch()
            c['stage']=2;c['at']=now
        elif c['stage']==2 and dt>1:
            for i,(p,mesh,head,foot) in enumerate(c['meshes']):
                inst=mesh.get_anim_instance()
                assert p.get_editor_property('is_crouched'),p.get_name()
                assert inst.get_editor_property('sneak_blend_alpha')>0.99,(p.get_name(),'alpha')
                height=mesh.get_socket_location(head).z-mesh.get_socket_location(foot).z
                u.log('SNEAK_TEST POSE '+p.get_name()+' standing='+str(c['heads'][i])+' crouched='+str(height))
                assert height<c['heads'][i]-15,(p.get_name(),'head did not lower')
                other='RightFoot' if mesh.does_socket_exist('RightFoot') else 'foot_r'
                feet=[mesh.get_socket_location(b).z for b in [foot,other]]
                u.log('SNEAK_TEST GROUND idle '+p.get_name()+' ankles_cm='+str(feet))
                assert min(feet)<18 and max(feet)<30,(p.get_name(),'floating idle feet',feet)
            u.log('SNEAK_TEST PASS: crouch idle and lowered pose on both network worlds')
            c['stage']=3;c['at']=now;c['poses']=[]
        elif c['stage']==3:
            hp.add_movement_input(u.Vector(1,0,0),1,False)
            cp.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<1:return
            for p,mesh,head,foot in c['meshes']:
                inst=mesh.get_anim_instance()
                assert inst.get_editor_property('sneak_move_speed')>30,(p.get_name(),'speed')
                assert inst.get_editor_property('sneak_blend_alpha')>0.99,(p.get_name(),'walk blend')
                c['poses'].append(mesh.get_socket_location(foot)-p.get_actor_location())
            c['stage']=4;c['at']=now
        elif c['stage']==4:
            hp.add_movement_input(u.Vector(1,0,0),1,False)
            cp.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<0.25:return
            for i,(p,mesh,head,foot) in enumerate(c['meshes']):
                pose=mesh.get_socket_location(foot)-p.get_actor_location()
                assert (pose-c['poses'][i]).length()>2,(p.get_name(),'static walking pose')
            u.log('SNEAK_TEST PASS: crouch walk has animated feet on both network worlds')
            hp.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            cp.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            c['stage']=5;c['at']=now
        elif c['stage']==5 and dt>1:
            for p,mesh,head,foot in c['meshes']:
                other='RightFoot' if mesh.does_socket_exist('RightFoot') else 'foot_r'
                feet=[mesh.get_socket_location(b).z for b in [foot,other]]
                u.log('SNEAK_TEST GROUND stopped '+p.get_name()+' ankles_cm='+str(feet))
                assert min(feet)<18 and max(feet)<30,(p.get_name(),'floating after stopping',feet)
            hp.un_crouch();cp.un_crouch()
            c['stage']=6;c['at']=now
        elif c['stage']==6 and dt>1:
            for p,mesh,head,foot in c['meshes']:
                assert not p.get_editor_property('is_crouched'),p.get_name()
                assert mesh.get_anim_instance().get_editor_property('sneak_blend_alpha')<0.01,p.get_name()
            u.log('SNEAK_TEST ALL PASSED: standing locomotion restored')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('SNEAK_TEST FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
