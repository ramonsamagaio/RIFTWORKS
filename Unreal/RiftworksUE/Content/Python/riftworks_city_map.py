from __future__ import annotations

import os
import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_CITY_"
CITY_ROOT = "/Game/Riftworks/CityKit"
CITY_MESH_DIR = f"{CITY_ROOT}/Meshes"
CITY_TEX_DIR = f"{CITY_ROOT}/Textures"
CITY_MAT_DIR = f"{CITY_ROOT}/Materials"
SOURCE_ROOT = os.path.join(rw.PROJECT_DIR, "ASSETS", "CityBuildings")
SOURCE_FBX = os.path.join(SOURCE_ROOT, "FBX (Unreal Engine)")
SOURCE_TEX = os.path.join(SOURCE_ROOT, "Textures")

SELECTED_MESHES = [
    "Building_Large_2", "Building_Medium_2_001", "Building_Small_1",
    "Street_2Lane", "Street_4WayIntersection", "Street_TIntersection", "Street_Curve_2Lane",
    "Sidewalk_Straight_3m", "Sidewalk_Corner_Round_3m", "Sidewalk_Planter",
    "Brick_Plain_1", "Brick_Plain_3", "Brick_Inset_Window", "Brick_Window_Square_Single",
    "Brick_InteriorWall_1", "Brick_InteriorWall_3", "Brick_Column_RedBricks",
    "Metal_FirstFloor_Wall", "Metal_FirstFloor_Window", "Metal_Plain_1", "Metal_Window",
    "Entrance_Concrete_2x2", "DoorFrame_Metal_Single", "Door_1", "Floor_4x4", "Roof_4x4",
    "Roof_Slate_Center", "Cornice_Brick_Center", "Cornice_Metal_Center", "Trim_FirstFloor_Wall",
    "Stairs_Entrance_Concrete", "Stairs_Rails_Metal", "Stairs_Rails_Metal_Straight_1",
    "Prop_ACUnit", "Prop_Bollard", "Prop_Drain", "Prop_ManholeCover", "Prop_Planter_Single",
]

TEXTURES = [
    "T_RedBrick_BaseColor.png", "T_Concrete_BaseColor.png", "T_MetalConcrete_BaseColor.png",
    "T_RoofSlate_BaseColor.png", "T_Trim_BaseColor.png", "T_Concrete_Asphalt_BaseColor.png",
]


def has_source_pack() -> bool:
    return os.path.isdir(SOURCE_FBX) and os.path.isfile(os.path.join(SOURCE_FBX, "Brick_Plain_1.fbx"))


def _ensure_dir(path):
    if not rw.asset_library.does_directory_exist(path):
        rw.asset_library.make_directory(path)


def _import_texture(filename):
    src = os.path.join(SOURCE_TEX, filename)
    name = os.path.splitext(filename)[0]
    asset_path = f"{CITY_TEX_DIR}/{name}"
    if rw.asset_library.does_asset_exist(asset_path):
        return rw.asset_library.load_asset(asset_path)
    if not os.path.isfile(src):
        return None
    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = CITY_TEX_DIR
    task.automated = True
    task.save = True
    task.replace_existing = False
    rw.asset_tools.import_asset_tasks([task])
    return rw.asset_library.load_asset(asset_path)


def _static_mesh_import_ui():
    ui = unreal.FbxImportUI()
    rw.safe_set(ui, "automated_import_should_detect_type", False)
    rw.safe_set(ui, "import_mesh", True)
    rw.safe_set(ui, "import_as_skeletal", False)
    rw.safe_set(ui, "import_animations", False)
    rw.safe_set(ui, "import_materials", False)
    rw.safe_set(ui, "import_textures", False)
    rw.safe_set(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    rw.safe_set(ui, "original_import_type", unreal.FBXImportType.FBXIT_STATIC_MESH)
    try:
        data = ui.get_editor_property("static_mesh_import_data")
        rw.safe_set(data, "combine_meshes", True)
        rw.safe_set(data, "auto_generate_collision", True)
        rw.safe_set(data, "generate_lightmap_u_vs", True)
    except Exception:
        pass
    return ui


def _import_mesh(name):
    path = f"{CITY_MESH_DIR}/{name}"
    if rw.asset_library.does_asset_exist(path):
        return rw.asset_library.load_asset(path)
    src = os.path.join(SOURCE_FBX, name + ".fbx")
    if not os.path.isfile(src):
        rw.warn(f"City mesh missing: {src}")
        return None
    task = unreal.AssetImportTask()
    task.filename = src
    task.destination_path = CITY_MESH_DIR
    task.automated = True
    task.save = True
    task.replace_existing = False
    task.options = _static_mesh_import_ui()
    rw.asset_tools.import_asset_tasks([task])
    return rw.asset_library.load_asset(path)


def import_city_assets():
    for folder in (CITY_ROOT, CITY_MESH_DIR, CITY_TEX_DIR, CITY_MAT_DIR):
        _ensure_dir(folder)
    textures = {os.path.splitext(name)[0]: _import_texture(name) for name in TEXTURES}
    meshes = {name: _import_mesh(name) for name in SELECTED_MESHES}
    rw.log(f"City kit import ready: {sum(1 for m in meshes.values() if m)} meshes")
    return meshes, textures


def _material(name, texture, roughness=0.72, metallic=0.0):
    path = f"{CITY_MAT_DIR}/{name}"
    if rw.asset_library.does_asset_exist(path):
        return rw.asset_library.load_asset(path)
    material = rw.asset_tools.create_asset(name, CITY_MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        return None
    if texture:
        sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -460, -80)
        rw.safe_set(sample, "texture", texture)
        unreal.MaterialEditingLibrary.connect_material_property(sample, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 120)
    rw.safe_set(rough, "r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    metal = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 220)
    rw.safe_set(metal, "r", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(material)
    rw.asset_library.save_asset(path, only_if_is_dirty=False)
    return material


def ensure_city_materials(textures):
    return {
        "brick": _material("M_City_Brick", textures.get("T_RedBrick_BaseColor"), 0.82, 0.0),
        "concrete": _material("M_City_Concrete", textures.get("T_Concrete_BaseColor"), 0.84, 0.0),
        "metal": _material("M_City_Metal", textures.get("T_MetalConcrete_BaseColor"), 0.55, 0.38),
        "roof": _material("M_City_Roof", textures.get("T_RoofSlate_BaseColor"), 0.78, 0.0),
        "trim": _material("M_City_Trim", textures.get("T_Trim_BaseColor"), 0.62, 0.22),
        "asphalt": _material("M_City_Asphalt", textures.get("T_Concrete_Asphalt_BaseColor"), 0.48, 0.0),
    }


def _mesh_size(mesh):
    if not mesh:
        return unreal.Vector(100.0, 100.0, 100.0)
    try:
        bounds = mesh.get_bounds()
        return unreal.Vector(bounds.box_extent.x * 2.0, bounds.box_extent.y * 2.0, bounds.box_extent.z * 2.0)
    except Exception:
        return unreal.Vector(100.0, 100.0, 100.0)


def _mesh_bottom(mesh):
    try:
        bounds = mesh.get_bounds()
        return float(bounds.origin.z - bounds.box_extent.z)
    except Exception:
        return -50.0


def _classify_material(name, mats):
    if name.startswith("Street_"):
        return mats.get("asphalt")
    if name.startswith(("Brick_", "Cornice_Brick")):
        return mats.get("brick")
    if name.startswith(("Metal_", "Prop_", "Stairs_Rails")):
        return mats.get("metal")
    if name.startswith("Roof_"):
        return mats.get("roof")
    if name.startswith(("Trim_", "Door")):
        return mats.get("trim")
    return mats.get("concrete")


def _spawn(actors, meshes, mats, name, label, location, *, yaw=0.0, scale=(1.0, 1.0, 1.0), material=None, collision=True):
    mesh = meshes.get(name)
    if not mesh:
        return None
    sx, sy, sz = scale
    x, y, base_z = location
    bottom = _mesh_bottom(mesh) * sz
    actor = actors.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(x, y, base_z - bottom),
        rw.rotator(yaw=yaw),
    )
    if not actor:
        return None
    actor.set_actor_label(PREFIX + label)
    actor.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    try:
        comp = actor.static_mesh_component
    except Exception:
        comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    if comp:
        try:
            comp.set_static_mesh(mesh)
        except Exception:
            rw.safe_set(comp, "static_mesh", mesh)
        try:
            comp.set_collision_profile_name("BlockAll" if collision else "NoCollision")
            comp.set_generate_overlap_events(False)
        except Exception:
            pass
        chosen = material or _classify_material(name, mats)
        if chosen:
            try:
                slots = max(1, comp.get_num_materials())
            except Exception:
                slots = 1
            for index in range(slots):
                try:
                    comp.set_material(index, chosen)
                except Exception:
                    pass
    return actor


def _cleanup_old_surface(actors):
    remove_prefixes = ("RIFT_CITY_", "RIFT_BEAUTY_", "RIFT_DRESS_", "RIFT_LANDMARK_", "RIFT_WEATHER_")
    keep_art = ("RIFT_ART_UG_", "RIFT_ART_Breach", "RIFT_ART_Moon", "RIFT_ART_Sky", "RIFT_ART_Fog")
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        if label.startswith(remove_prefixes):
            actors.destroy_actor(actor)
        elif label.startswith("RIFT_ART_") and not label.startswith(keep_art):
            actors.destroy_actor(actor)


def _road_axis(mesh):
    size = _mesh_size(mesh)
    if size.x >= size.y:
        return 90.0, max(size.x, 100.0)
    return 0.0, max(size.y, 100.0)


def _build_roads(actors, meshes, mats):
    street = meshes.get("Street_2Lane")
    yaw, step = _road_axis(street)
    step *= 0.985
    for i in range(-6, 7):
        if i == 0:
            continue
        _spawn(actors, meshes, mats, "Street_2Lane", f"MainRoad_{i:+03d}", (0.0, i * step, 0.0), yaw=yaw)
    _spawn(actors, meshes, mats, "Street_4WayIntersection", "CentralIntersection", (0.0, 0.0, 0.0), yaw=yaw)
    _spawn(actors, meshes, mats, "Street_TIntersection", "SouthTJunction", (0.0, -step * 4.0, 0.0), yaw=yaw)
    _spawn(actors, meshes, mats, "Street_Curve_2Lane", "NorthCurve", (0.0, step * 6.2, 0.0), yaw=yaw)

    # Detail props from the pack establish believable scale without hand-made cubes.
    for i, y in enumerate((-2800, -1700, -620, 650, 1760, 2870)):
        side = -1 if i % 2 == 0 else 1
        _spawn(actors, meshes, mats, "Prop_Drain", f"Drain_{i}", (side * 620.0, y, 1.0), yaw=90.0, collision=False)
    for i, y in enumerate((-2300, 1180, 2450)):
        _spawn(actors, meshes, mats, "Prop_ManholeCover", f"Manhole_{i}", (130.0 if i % 2 else -120.0, y, 2.0), collision=False)
    for i, y in enumerate((-3400, -3250, 3200, 3370)):
        _spawn(actors, meshes, mats, "Prop_Bollard", f"Bollard_{i}", (-720.0 if i < 2 else 720.0, y, 0.0))


def _wall_dimensions(mesh):
    size = _mesh_size(mesh)
    horizontal = max(size.x, size.y)
    return max(horizontal, 250.0), max(size.z, 260.0)


def _build_modular_building(actors, meshes, mats, key, center, width_modules, depth_modules, levels, style="brick", wide_entry=False):
    wall_name = "Metal_FirstFloor_Wall" if style == "metal" else "Brick_Plain_1"
    window_name = "Metal_FirstFloor_Window" if style == "metal" else "Brick_Window_Square_Single"
    wall_mesh = meshes.get(wall_name) or meshes.get("Brick_Plain_1")
    module, level_h = _wall_dimensions(wall_mesh)
    cx, cy, base_z = center
    width = width_modules * module
    depth = depth_modules * module

    floor_mesh = meshes.get("Floor_4x4")
    floor_size = _mesh_size(floor_mesh)
    floor_scale = (width / max(1.0, floor_size.x), depth / max(1.0, floor_size.y), 1.0)
    _spawn(actors, meshes, mats, "Floor_4x4", f"{key}_Floor", (cx, cy, base_z), scale=floor_scale, material=mats.get("concrete"))

    entry_width_slots = 2 if wide_entry else 1
    entry_start = (width_modules - entry_width_slots) // 2

    for level_idx in range(levels):
        z = base_z + level_idx * level_h
        for ix in range(width_modules):
            x = cx - width * 0.5 + module * (ix + 0.5)
            is_entry = level_idx == 0 and entry_start <= ix < entry_start + entry_width_slots
            if not is_entry:
                piece = window_name if (ix + level_idx) % 2 == 0 else wall_name
                _spawn(actors, meshes, mats, piece, f"{key}_South_{level_idx}_{ix}", (x, cy - depth * 0.5, z), yaw=0.0)
            piece_back = window_name if (ix + level_idx) % 3 != 1 else wall_name
            _spawn(actors, meshes, mats, piece_back, f"{key}_North_{level_idx}_{ix}", (x, cy + depth * 0.5, z), yaw=180.0)

        for iy in range(depth_modules):
            y = cy - depth * 0.5 + module * (iy + 0.5)
            piece = window_name if (iy + level_idx) % 2 else wall_name
            _spawn(actors, meshes, mats, piece, f"{key}_West_{level_idx}_{iy}", (cx - width * 0.5, y, z), yaw=90.0)
            _spawn(actors, meshes, mats, piece, f"{key}_East_{level_idx}_{iy}", (cx + width * 0.5, y, z), yaw=-90.0)

        if level_idx < levels - 1:
            _spawn(actors, meshes, mats, "Floor_4x4", f"{key}_UpperFloor_{level_idx}", (cx, cy, z + level_h), scale=floor_scale, material=mats.get("concrete"))

    roof_mesh = meshes.get("Roof_4x4")
    roof_size = _mesh_size(roof_mesh)
    roof_scale = (width / max(1.0, roof_size.x), depth / max(1.0, roof_size.y), 1.0)
    _spawn(actors, meshes, mats, "Roof_4x4", f"{key}_Roof", (cx, cy, base_z + levels * level_h), scale=roof_scale, material=mats.get("roof"))

    # Door frame is centered in the real opening, so the shell is physically enterable.
    _spawn(actors, meshes, mats, "DoorFrame_Metal_Single", f"{key}_DoorFrame", (cx, cy - depth * 0.5 - 2.0, base_z), yaw=0.0, material=mats.get("trim"), collision=False)
    _spawn(actors, meshes, mats, "Door_1", f"{key}_DoorOpen", (cx - module * 0.42, cy - depth * 0.5 - 8.0, base_z), yaw=92.0, material=mats.get("trim"), collision=False)

    # Internal rooms use the kit's actual interior-wall module, with a clear center gap.
    inner = "Brick_InteriorWall_1"
    for ix in range(-2, 3):
        if ix == 0:
            continue
        _spawn(actors, meshes, mats, inner, f"{key}_Interior_{ix}", (cx + ix * module * 0.72, cy + depth * 0.12, base_z), yaw=0.0, material=mats.get("brick"))

    # Rooftop machinery from the same pack.
    for index in range(max(1, width_modules // 3)):
        _spawn(actors, meshes, mats, "Prop_ACUnit", f"{key}_AC_{index}", (cx - width * 0.25 + index * module * 1.4, cy + depth * 0.12, base_z + levels * level_h + 20.0), material=mats.get("metal"))


def _build_district(actors, meshes, mats):
    _build_modular_building(actors, meshes, mats, "Workshop", (1550.0, 950.0, 18.0), 6, 5, 1, style="metal", wide_entry=True)
    _build_modular_building(actors, meshes, mats, "CornerStore", (-1480.0, 1250.0, 18.0), 4, 4, 1, style="brick")
    _build_modular_building(actors, meshes, mats, "Motel", (-1700.0, -1450.0, 18.0), 6, 4, 2, style="brick", wide_entry=True)

    # Open utility yard instead of another opaque box.
    for i, x in enumerate((1050.0, 1500.0, 1950.0, 2400.0)):
        _spawn(actors, meshes, mats, "Metal_Column_Bottom" if meshes.get("Metal_Column_Bottom") else "Metal_Plain_1", f"SubstationPost_{i}", (x, -2350.0, 0.0), material=mats.get("metal"))
        _spawn(actors, meshes, mats, "Prop_ACUnit", f"SubstationUnit_{i}", (x, -1750.0, 0.0), material=mats.get("metal"))

    # Distant complete buildings create urban mass while the important buildings stay modular/enterable.
    skyline = [
        ("Building_Large_2", (-5200.0, 2100.0, 0.0), 12.0),
        ("Building_Medium_2_001", (4700.0, 2600.0, 0.0), -18.0),
        ("Building_Small_1", (-4500.0, -2300.0, 0.0), 28.0),
        ("Building_Medium_2_001", (4900.0, -3200.0, 0.0), 10.0),
        ("Building_Large_2", (-5100.0, -5200.0, 0.0), -8.0),
    ]
    for index, (mesh_name, loc, yaw) in enumerate(skyline):
        _spawn(actors, meshes, mats, mesh_name, f"Skyline_{index}", loc, yaw=yaw)


def _build_subway_headhouse(actors, meshes, mats):
    # Existing accessibility stairs remain authoritative below grade. The pack now
    # gives them an architectural reason to exist instead of a staircase in a field.
    _spawn(actors, meshes, mats, "Entrance_Concrete_2x2", "SubwayEntrance", (0.0, -4070.0, 0.0), yaw=180.0, material=mats.get("concrete"))
    _spawn(actors, meshes, mats, "Stairs_Entrance_Concrete", "SubwayStairTop", (0.0, -4260.0, -5.0), yaw=180.0, material=mats.get("concrete"))
    for side in (-1, 1):
        _spawn(actors, meshes, mats, "Stairs_Rails_Metal_Straight_1", f"SubwayRail_{side}", (side * 310.0, -4270.0, 20.0), yaw=180.0, material=mats.get("metal"), collision=False)
        _spawn(actors, meshes, mats, "Prop_Bollard", f"SubwayBollard_{side}", (side * 430.0, -3970.0, 0.0), material=mats.get("metal"))


def _ensure_loot_blueprint():
    path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftLootContainer"
    rw.create_blueprint("BP_RiftLootContainer", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftLootContainer")
    return path


def _spawn_loot(actors, path, label, location, tier, seed, name):
    cls = rw.blueprint_class(path)
    if not cls:
        return None
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), rw.rotator())
    if not actor:
        return None
    actor.set_actor_label(PREFIX + "LOOT_" + label)
    rw.safe_set(actor, "container_name", name)
    rw.safe_set(actor, "loot_tier", tier)
    rw.safe_set(actor, "loot_seed", seed)
    rw.safe_set(actor, "capacity_slots", 20)
    return actor


def _stage_loot(actors):
    path = _ensure_loot_blueprint()
    placements = [
        ("SafehouseSupplies", (-1850.0, 1410.0, 24.0), 1, 1101, "Safehouse Supplies"),
        ("WorkshopToolCrate", (1980.0, 1300.0, 24.0), 2, 2202, "Workshop Tool Crate"),
        ("WorkshopParts", (1180.0, 1450.0, 24.0), 2, 2203, "Machine Parts"),
        ("MotelRoomCache", (-2050.0, -1320.0, 24.0), 2, 3304, "Abandoned Room Cache"),
        ("SubstationLocker", (2100.0, -1650.0, 20.0), 3, 4405, "Grid Service Locker"),
        ("MetroMaintenance", (-520.0, -7100.0, -900.0), 3, 5506, "Metro Maintenance Case"),
        ("BreachResearch", (720.0, -9800.0, -1310.0), 4, 6607, "Research Lockbox"),
    ]
    for args in placements:
        _spawn_loot(actors, path, *args)


def apply_all():
    if not has_source_pack():
        rw.warn(f"City pack not found at {SOURCE_FBX}")
        return False
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return False

    meshes, textures = import_city_assets()
    mats = ensure_city_materials(textures)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    _cleanup_old_surface(actors)
    _build_roads(actors, meshes, mats)
    _build_district(actors, meshes, mats)
    _build_subway_headhouse(actors, meshes, mats)
    _stage_loot(actors)

    level.save_current_level()
    try:
        rw.asset_library.save_directory(CITY_ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    rw.log("MODULAR CITY REBUILD complete: real streets, enterable modular buildings, skyline, subway headhouse and loot containers")
    return True


if __name__ == "__main__":
    apply_all()
