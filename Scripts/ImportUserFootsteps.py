"""Import supplied left/right MP3s, export PCM WAV, then reimport mono source."""
import unreal
import wave
import struct
from pathlib import Path
folder=Path(unreal.Paths.project_content_dir())/'Audio'/'Footsteps'/'Source'
assets=unreal.AssetToolsHelpers.get_asset_tools()
decoded={}
for side in ['Left','Right']:
    name='SW_Footstep'+side
    package='/Game/Audio/Footsteps/'+name
    task=unreal.AssetImportTask()
    task.filename=str((folder/(side.lower()+'.mp3')).resolve())
    task.destination_path='/Game/Audio/Footsteps'
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=False
    assets.import_asset_tasks([task])
    sound=unreal.load_asset(package)
    assert sound,package
    output=folder/('Footstep'+side+'.wav')
    export=unreal.AssetExportTask()
    export.object=sound
    export.filename=str(output.resolve())
    export.automated=True
    export.prompt=False
    export.replace_identical=True
    assert unreal.Exporter.run_asset_export_task(export),'WAV export failed'
    with wave.open(str(output),'rb') as f:
        channels=f.getnchannels();width=f.getsampwidth();rate=f.getframerate();frames=f.getnframes();pcm=f.readframes(frames)
    assert width==2,('Expected 16 bit PCM',width)
    samples=struct.unpack('<'+'h'*(len(pcm)//2),pcm)
    mono=[int(sum(samples[k:k+channels])/channels) for k in range(0,len(samples),channels)]
    assert max(abs(v) for v in mono)>100,'silent source'
    decoded[side]=(rate,mono)

# Match the old placeholder's impact level while retaining the two clips' relative level.
gain=0.35*32767/max(abs(v) for rate,mono in decoded.values() for v in mono)
for side,(rate,mono) in decoded.items():
    threshold=max(abs(v) for v in mono)*0.03
    active=[i for i,v in enumerate(mono) if abs(v)>threshold]
    start=max(0,active[0]-int(rate*0.02))
    end=min(len(mono),active[-1]+int(rate*0.06))
    mono=[int(max(-32767,min(32767,v*gain))) for v in mono[start:end]]
    output=folder/('Footstep'+side+'.wav')
    with wave.open(str(output),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate)
        f.writeframes(struct.pack('<'+'h'*len(mono),*mono))
    task=unreal.AssetImportTask()
    task.filename=str(output.resolve())
    task.destination_path='/Game/Audio/Footsteps'
    task.destination_name='SW_Footstep'+side
    task.automated=True
    task.replace_existing=True
    task.save=True
    assets.import_asset_tasks([task])
    sound=unreal.load_asset('/Game/Audio/Footsteps/SW_Footstep'+side)
    unreal.EditorAssetLibrary.save_loaded_asset(sound)
    unreal.log('USER_FOOTSTEP_IMPORT PASS '+side+' duration='+str(len(mono)/rate)+' rate='+str(rate)+' mono peak='+str(max(abs(v) for v in mono)))
