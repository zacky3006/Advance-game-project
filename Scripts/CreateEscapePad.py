import unreal

lib = unreal.MaterialEditingLibrary
for name, xray in [('M_EscapePad', False), ('M_EscapePadXRay', True)]:
    path = '/Game/UI/Materials/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, '/Game/UI/Materials', unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('disable_depth_test', xray)
    color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'GlowColor')
    color.set_editor_property('default_value', unreal.LinearColor(0.04, 1.0, 0.18, 1.0) if xray else unreal.LinearColor(1.0, 0.015, 0.008, 1.0))
    fresnel = lib.create_material_expression(mat, unreal.MaterialExpressionFresnel)
    fresnel.set_editor_property('exponent', 2.5)
    fresnel.set_editor_property('base_reflect_fraction', 0.04)
    strength = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply)
    strength.set_editor_property('const_b', 5.0)
    lib.connect_material_expressions(fresnel, '', strength, 'A')
    glow = lib.create_material_expression(mat, unreal.MaterialExpressionAdd)
    glow.set_editor_property('const_b', 2.0)
    lib.connect_material_expressions(strength, '', glow, 'A')
    tint = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply)
    lib.connect_material_expressions(color, '', tint, 'A')
    lib.connect_material_expressions(glow, '', tint, 'B')
    lib.connect_material_property(tint, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity_rim = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply)
    opacity_rim.set_editor_property('const_b', 0.45)
    lib.connect_material_expressions(fresnel, '', opacity_rim, 'A')
    opacity = lib.create_material_expression(mat, unreal.MaterialExpressionAdd)
    opacity.set_editor_property('const_b', 0.12 if xray else 0.32)
    lib.connect_material_expressions(opacity_rim, '', opacity, 'A')
    lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
print('Escape pad materials ready')
