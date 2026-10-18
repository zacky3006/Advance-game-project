"""Two-player PIE: spatial footstep playback for host, client, and AI."""
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
        # Observe real spawned AudioComponents, including replicated multicast playback.
        for world in ws:
            side='server' if world==s else 'client'
            for actor in u.GameplayStatics.get_all_actors_of_class(world,u.ArenaGameCharacter):
                ps=actor.get_editor_property('player_state')
                label=str(ps.get_editor_property('player_id')) if ps else 'AI'
                for audio in actor.get_components_by_class(u.AudioComponent):
                    if not audio.component_has_tag('FootstepAudio') or not audio.is_playing():continue
                    key=audio.get_path_name()
                    if key in c['seen']:continue
                    c['seen'].add(key)
                    sound=audio.get_editor_property('sound')
                    assert sound and sound.get_name() in ['SW_FootstepLeft','SW_FootstepRight'], 'old sound still selected'
                    foot_y=audio.get_editor_property('relative_location').y
                    assert (sound.get_name()=='SW_FootstepLeft') == (foot_y<0), 'wrong sound for left/right foot'
                    c.setdefault('clips',set()).add(sound.get_name())
                    att=audio.get_editor_property('attenuation_settings')
                    assert att and att.get_editor_property('attenuation').get_editor_property('spatialize')
                    c['counts'][(side,label)]=c['counts'].get((side,label),0)+1
                    if actor==hp:c['volumes'].setdefault(c['stage'],[]).append(audio.get_editor_property('volume_multiplier'))
        dt=now-c['at']
        if c['stage']==1 and dt>1:
            c['baseline']=len(c['seen']);c['stage']=2;c['at']=now
        elif c['stage']==2:
            for p in [hp,cp,c['guard']]:p.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<2.2:return
            for side in ['server','client']:
                for label in [str(c['hostid']),str(c['clientid']),'AI']:
                    assert c['counts'].get((side,label),0)>=2,(side,label,c['counts'])
            assert len(c.get('clips',set()))==2, 'both user clips must play'
            u.log('FOOTSTEP_TEST PASS: user left/right sounds match foot sides for host, client and AI on both network worlds')
            c['walk_count']=len(c['volumes'].get(2,[]))
            hp.call_method('ServerSetSprintState',args=(True,))
            c['stage']=3;c['at']=now
        elif c['stage']==3:
            hp.add_movement_input(u.Vector(-1,0,0),1,False)
            if dt<2.2:return
            assert c['volumes'].get(3), 'no sprint footsteps'
            assert min(c['volumes'][3])>max(c['volumes'][2])
            assert len(c['volumes'][3])>=c['walk_count'],(c['volumes'],c['walk_count'])
            u.log('FOOTSTEP_TEST PASS: sprint footsteps are louder and faster')
            hp.call_method('ServerSetSprintState',args=(False,))
            hp.call_method('ServerSetSneakState',args=(True,))
            c['stage']=4;c['at']=now
        elif c['stage']==4:
            hp.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<2.2:return
            assert c['volumes'].get(4),'no sneak footsteps'
            assert max(c['volumes'][4])<min(c['volumes'][2])
            u.log('FOOTSTEP_TEST PASS: sneaking remains audible at reduced volume')
            hp.call_method('ServerSetSneakState',args=(False,))
            for p in [hp,sp,cp,c['guard']]:p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            c['stage']=5;c['at']=now
        elif c['stage']==5 and dt>0.6:
            c['stopped_count']=len(c['seen']);c['stage']=6;c['at']=now
        elif c['stage']==6 and dt>0.8:
            assert len(c['seen'])==c['stopped_count'],'new footsteps while stationary'
            hp.set_actor_location(hp.get_actor_location()+u.Vector(0,0,400),False,True)
            hp.get_component_by_class(u.CharacterMovementComponent).set_movement_mode(u.MovementMode.MOVE_FALLING)
            c['stage']=7;c['at']=now;c['air_count']=len(c['seen'])
        elif c['stage']==7:
            hp.add_movement_input(u.Vector(1,0,0),1,False)
            if dt<0.4:return
            assert len(c['seen'])==c['air_count'],'footstep while airborne'
            u.log('FOOTSTEP_TEST PASS: stationary and airborne movement produce no new footsteps')
            u.unregister_slate_post_tick_callback(c['handle'])
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
            c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
    except Exception as ex:
        u.log_error('FOOTSTEP_TEST FAIL stage='+str(c['stage'])+' '+repr(ex))
        u.unregister_slate_post_tick_callback(c['handle'])
        u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
        c['handle']=u.register_slate_post_tick_callback(lambda dt,ue=u:ue.SystemLibrary.quit_editor())
c['handle']=unreal.register_slate_post_tick_callback(tick)
level.editor_request_begin_play()
