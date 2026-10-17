import unreal

path = "/Game/UI/Materials/M_LootThumbnailBackdrop"
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LootThumbnailBackdrop", "/Game/UI/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(0.12, 0.12, 0.12, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
