import unreal
path = '/Game/UI/Materials/M_IntroVideo'
texture_path = '/Game/UI/Materials/MT_IntroPlaceholder'
texture = unreal.load_asset(texture_path) if unreal.EditorAssetLibrary.does_asset_exist(texture_path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('MT_IntroPlaceholder', '/Game/UI/Materials', unreal.MediaTexture, unreal.MediaTextureFactoryNew())
texture.set_editor_property('new_style_output', True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    material = unreal.load_asset(path)
    sample = unreal.find_object(material, 'MaterialExpressionTextureSampleParameter2D_0')
    if not sample:
        raise RuntimeError('Expected Video texture parameter was not found')
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_IntroVideo', '/Game/UI/Materials', unreal.Material, unreal.MaterialFactoryNew())
    sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D)
material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
sample.set_editor_property('parameter_name', 'Video')
sample.set_editor_property('texture', texture)
sample.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
print('Intro video material ready')
