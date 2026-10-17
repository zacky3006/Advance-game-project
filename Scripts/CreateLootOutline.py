"""Run once with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Creates the runtime outline material; does not modify maps or project settings.
"""
import unreal

path = "/Game/UI/Materials/M_LootOutline"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("Loot outline material already exists")
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LootOutline", "/Game/UI/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    library = unreal.MaterialEditingLibrary

    color = library.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, -200)
    color.set_editor_property("constant", unreal.LinearColor(1.0, 0.72, 0.015, 1.0))
    library.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Inverted hull: hide front faces, draw expanded back faces behind the original mesh.
    sign = library.create_material_expression(material, unreal.MaterialExpressionTwoSidedSign, -600, 0)
    invert = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 0)
    invert.set_editor_property("const_b", -1.0)
    library.connect_material_expressions(sign, "", invert, "A")
    library.connect_material_property(invert, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    normal = library.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, -600, 200)
    width = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 350)
    width.set_editor_property("parameter_name", "OutlineWidth")
    width.set_editor_property("default_value", 1.25)
    expand = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 200)
    library.connect_material_expressions(normal, "", expand, "A")
    library.connect_material_expressions(width, "", expand, "B")
    library.connect_material_property(expand, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    library.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("Created and saved " + path)
