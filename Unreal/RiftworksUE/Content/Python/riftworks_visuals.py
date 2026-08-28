from __future__ import annotations

import unreal
import riftworks_setup as rw

MAT_WORLD_DIR = f"{rw.MAT_DIR}/World"
MAT_GAMEPLAY_DIR = f"{rw.MAT_DIR}/Gameplay"


def _ensure_dir(path: str) -> None:
    if not rw.asset_library.does_directory_exist(path):
        rw.asset_library.make_directory(path)


def _constant3(material, color: unreal.LinearColor, x: int, y: int):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, x, y)
    rw.safe_set(node, "constant", color)
    return node


def _constant(material, value: float, x: int, y: int):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    rw.safe_set(node, "r", value)
    return node


def _procedural_variation(material, base, rough, amount: float):
    """Cheap world-space breakup for placeholder geometry. Fails soft if an expression changes in UE 5.8."""
    if amount <= 0.0:
        return base, rough
    try:
        world = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -860, -220)
        noise = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionNoise, -620, -220)
        rw.safe_set(noise, "scale", 0.0065)
        rw.safe_set(noise, "quality", 1)
        rw.safe_set(noise, "levels", 3)
        rw.safe_set(noise, "output_min", max(0.40, 1.0 - amount))
        rw.safe_set(noise, "output_max", 1.0 + amount * 0.55)
        unreal.MaterialEditingLibrary.connect_material_expressions(world, "", noise, "Position")

        color_mul = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, -65)
        unreal.MaterialEditingLibrary.connect_material_expressions(base, "", color_mul, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(noise, "", color_mul, "B")

        rough_mul = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 130)
        unreal.MaterialEditingLibrary.connect_material_expressions(rough, "", rough_mul, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(noise, "", rough_mul, "B")
        return color_mul, rough_mul
    except Exception as exc:
        rw.warn(f"Procedural material breakup skipped: {exc}")
        return base, rough


def create_surface_material(name: str, color: tuple[float, float, float], roughness: float = 0.8,
                            metallic: float = 0.0, emissive: tuple[float, float, float] | None = None,
                            variation: float = 0.0):
    path = f"{MAT_WORLD_DIR}/{name}"
    if rw.asset_library.does_asset_exist(path):
        return rw.asset_library.load_asset(path)

    material = rw.asset_tools.create_asset(name, MAT_WORLD_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        return None

    base = _constant3(material, unreal.LinearColor(color[0], color[1], color[2], 1.0), -520, -40)
    rough = _constant(material, roughness, -520, 130)
    metal = _constant(material, metallic, -520, 230)
    varied_base, varied_rough = _procedural_variation(material, base, rough, variation)
    unreal.MaterialEditingLibrary.connect_material_property(varied_base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(varied_rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    if emissive:
        glow = _constant3(material, unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0), -520, 340)
        unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    rw.asset_library.save_asset(path, only_if_is_dirty=False)
    return material


def ensure_material_library() -> dict[str, object]:
    _ensure_dir(MAT_WORLD_DIR)
    _ensure_dir(MAT_GAMEPLAY_DIR)

    # key: name, base color, roughness, metallic, emissive, procedural breakup
    specs = {
        "ground": ("M_Ground_MossDark", (0.045, 0.052, 0.036), 0.94, 0.0, None, 0.24),
        "asphalt": ("M_Asphalt_WetNight", (0.020, 0.027, 0.034), 0.34, 0.0, None, 0.19),
        "concrete": ("M_Concrete_ColdDirty", (0.105, 0.115, 0.105), 0.86, 0.0, None, 0.18),
        "concrete_dark": ("M_Concrete_Deep", (0.050, 0.058, 0.062), 0.90, 0.0, None, 0.21),
        "metal": ("M_Metal_Industrial", (0.055, 0.070, 0.075), 0.48, 0.62, None, 0.08),
        "rust": ("M_Metal_Rust", (0.175, 0.064, 0.026), 0.78, 0.18, None, 0.28),
        "glass": ("M_Glass_BlackBlue", (0.008, 0.022, 0.035), 0.14, 0.05, None, 0.03),
        "trunk": ("M_Wood_Charred", (0.075, 0.050, 0.033), 0.96, 0.0, None, 0.23),
        "foliage": ("M_Foliage_Night", (0.025, 0.072, 0.040), 0.94, 0.0, None, 0.16),
        "road_white": ("M_RoadMark_White", (0.39, 0.40, 0.35), 0.70, 0.0, None, 0.10),
        "road_yellow": ("M_RoadMark_Yellow", (0.48, 0.31, 0.055), 0.72, 0.0, None, 0.10),
        "hazard": ("M_Hazard_Amber", (0.56, 0.20, 0.025), 0.58, 0.08, None, 0.08),
        "lamp": ("M_Lamp_AmberGlow", (0.38, 0.19, 0.055), 0.32, 0.0, (5.2, 2.2, 0.55), 0.0),
        "emergency": ("M_Emergency_RedGlow", (0.24, 0.018, 0.012), 0.36, 0.0, (5.5, 0.18, 0.08), 0.0),
        "breach": ("M_Breach_VioletGlow", (0.055, 0.018, 0.12), 0.28, 0.22, (1.5, 0.24, 6.5), 0.08),
        "breach_dark": ("M_Breach_BlackStone", (0.026, 0.016, 0.041), 0.58, 0.18, None, 0.26),
        "rubber": ("M_Rubber_Dark", (0.014, 0.016, 0.017), 0.93, 0.0, None, 0.06),
        "assembly": ("M_Assembly_Steel", (0.075, 0.105, 0.110), 0.55, 0.66, None, 0.07),
        "assembly_motor": ("M_Assembly_Motor", (0.26, 0.085, 0.018), 0.53, 0.48, None, 0.10),
        "salvage": ("M_Salvage_Utility", (0.115, 0.125, 0.105), 0.70, 0.34, None, 0.12),
        "colossus": ("M_Colossus_Graphite", (0.033, 0.025, 0.052), 0.48, 0.22, None, 0.14),
    }

    result = {}
    for key, (name, color, roughness, metallic, emissive, variation) in specs.items():
        result[key] = create_surface_material(name, color, roughness, metallic, emissive, variation)
    rw.log(f"Visual material library ready: {len(result)} materials")
    return result


def _set_component_material(cdo, component_property: str, material) -> None:
    if not cdo or not material:
        return
    try:
        component = cdo.get_editor_property(component_property)
        if component:
            component.set_material(0, material)
    except Exception:
        pass


def apply_gameplay_materials(materials: dict[str, object]) -> None:
    mappings = [
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftSalvage", "mesh", "salvage"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBaseBeacon", "mesh", "metal"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftGenerator", "mesh", "rust"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBattery", "mesh", "metal"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFloodlight", "mesh", "metal"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachRepulsion", "mesh", "breach_dark"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachAttraction", "mesh", "breach_dark"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachLuminance", "mesh", "breach_dark"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachGravity", "mesh", "breach_dark"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASPlatform", "physics_mesh", "assembly"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASBeam", "physics_mesh", "assembly"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASWheel", "physics_mesh", "rubber"),
        (f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASMotorWheel", "physics_mesh", "assembly_motor"),
    ]
    for path, component_prop, material_key in mappings:
        cdo = rw.blueprint_cdo(path)
        if not cdo:
            continue
        _set_component_material(cdo, component_prop, materials.get(material_key))
        rw.asset_library.save_asset(path, only_if_is_dirty=False)

    colossus_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftMannequinColossus"
    colossus = rw.blueprint_cdo(colossus_path)
    if colossus and materials.get("colossus"):
        _set_component_material(colossus, "mesh", materials["colossus"])
        rw.asset_library.save_asset(colossus_path, only_if_is_dirty=False)


def apply_all() -> dict[str, object]:
    mats = ensure_material_library()
    apply_gameplay_materials(mats)
    rw.log("Gameplay placeholder materials applied")
    return mats


if __name__ == "__main__":
    apply_all()
