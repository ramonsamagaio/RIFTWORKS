from __future__ import annotations

import math
import unreal

import riftworks_setup as rw
import riftworks_city_map as base

# Final authored CityBuildings pass. The old hand-built passes are still useful
# for atmosphere and the deep Breach route, but this script owns the playable
# surface, the subway stair geometry and all CityKit loot placement.
PREFIX = base.PREFIX
has_source_pack = base.has_source_pack

EXTRA_MESHES = [
    "Brick_90Angle_L", "Brick_90Angle_R", "Brick_BottomTrim",
    "Brick_Column_Small", "Brick_Column_TrimBricks",
    "Brick_CornerColumn_Bottom", "Brick_CornerColumn_Cap",
    "Brick_CornerColumn_CapShort", "Brick_CornerColumn_Center",
    "Brick_CornerColumn_Top", "Brick_Corner_Plain",
    "Brick_HalfColumn_Bottom", "Brick_HalfColumn_Center", "Brick_HalfColumn_Top",
    "Brick_Ornament_Horizontal", "Brick_RedWhite_DoubleWindow",
    "Brick_TopTrim", "Brick_TopTrim_Corner", "Brick_Window_CurvedDouble",
    "Cornice_Brick_90Angle_L", "Cornice_Brick_90Angle_R",
    "Cornice_Brick_L", "Cornice_Brick_R",
    "Cornice_Metal_90Angle_L", "Cornice_Metal_90Angle_R",
    "Cornice_Metal_L", "Cornice_Metal_R",
    "Cornice_Trim_Center", "Cornice_Trim_L", "Cornice_Trim_R",
    "DoorFrame_Trim", "Door_2", "Door_3", "Entrance_Concrete_2x1",
    "Floor_2x2", "Floor_Inset",
    "Metal_Column_Center", "Metal_Column_Top",
    "Metal_Column_Small_Bottom", "Metal_Column_Small_Center", "Metal_Column_Small_Top",
    "Metal_FirstFloor_Wall_1", "Metal_FullWindow", "Metal_Plain_3", "Metal_Window_Half",
    "Roof_2x2", "Roof_SlateCornice_Center", "Roof_SlateCornice_Corner",
    "Roof_SlateCornice_Window_1", "Roof_Slate_Corner", "Roof_Slate_Window_1",
    "Sidewalk_Corner_Flat_3m", "Sidewalk_NoCurb_3m",
    "Stairs_Rails_Metal_Straight_2",
    "Street_2Lane_noSidewalk", "Street_4Lane", "Street_Asphalt_6x6", "Street_Asphalt_9x9",
    "Trim_Column_Bottom", "Trim_Column_Center", "Trim_Column_Top",
    "Trim_Corner", "Trim_FirstFloor_Window_001", "Trim_FirstFloor_Window_Columns",
    "Trim_Plain_3", "Trim_Wall_Guard", "Trim_Window",
]


def _extend_selection():
    for name in EXTRA_MESHES:
        if name not in base.SELECTED_MESHES:
            base.SELECTED_MESHES.append(name)


def _label(actor):
    try:
        return actor.get_actor_label()
    except Exception:
        return ""


def _z(actor):
    try:
        return float(actor.get_actor_location().z)
    except Exception:
        return 0.0


def _cleanup_authoring_layers(actors):
    """Remove stale surface art without erasing the deep underground gameplay."""
    removed = 0
    surface_prefixes = (
        "RIFT_BEAUTY_", "RIFT_DRESS_", "RIFT_WEATHER_", "RIFT_LIVED_",
        "RIFT_LANDMARK_", "RIFT_EMERGENCY_", "RIFT_STAGE_",
    )
    for actor in list(actors.get_all_level_actors()):
        label = _label(actor)
        if not label:
            continue
        actor_z = _z(actor)
        remove = False

        # CityKit owns all of its previous output, so rebuild it idempotently.
        if label.startswith(PREFIX):
            remove = True

        # The accessibility pass used cube stairs. CityKit stairs replace both
        # surface-to-station and station-to-Breach transitions completely.
        elif label.startswith("RIFT_ACCESS_"):
            remove = True
        elif label.startswith("RIFT_ART_UG_Step") or label.startswith("RIFT_ART_UG_DeepStep"):
            remove = True

        # Cosmetic legacy passes are removed only near the surface. Their deep
        # underground dressing remains until a dedicated underground kit pass.
        elif actor_z > -620.0 and label.startswith(surface_prefixes):
            remove = True
        elif actor_z > -620.0 and label.startswith("RIFT_ART_"):
            keep = (
                "RIFT_ART_Moon", "RIFT_ART_Sky", "RIFT_ART_Fog",
                "RIFT_ART_Atmosphere",
            )
            remove = not label.startswith(keep)

        if remove:
            try:
                actors.destroy_actor(actor)
                removed += 1
            except Exception:
                pass
    return removed


def _axis_yaw(mesh, desired_axis="y"):
    size = base._mesh_size(mesh)
    long_is_x = size.x >= size.y
    if desired_axis == "y":
        return 90.0 if long_is_x else 0.0
    return 0.0 if long_is_x else 90.0


def _line(actors, meshes, mats, mesh_name, label, start, end, z, *, material=None, collision=True):
    mesh = meshes.get(mesh_name)
    if not mesh:
        return 0
    x0, y0 = start
    x1, y1 = end
    dx, dy = x1 - x0, y1 - y0
    length = max(1.0, math.hypot(dx, dy))
    size = base._mesh_size(mesh)
    piece_length = max(80.0, max(size.x, size.y) * 0.94)
    count = max(1, int(math.ceil(length / piece_length)))
    axis = "x" if abs(dx) >= abs(dy) else "y"
    yaw = _axis_yaw(mesh, axis)
    spawned = 0
    for i in range(count):
        t = (i + 0.5) / count
        actor = base._spawn(
            actors, meshes, mats, mesh_name, f"{label}_{i:02d}",
            (x0 + dx * t, y0 + dy * t, z), yaw=yaw,
            material=material, collision=collision,
        )
        spawned += 1 if actor else 0
    return spawned


def _build_sidewalks(actors, meshes, mats):
    spawned = 0
    sidewalk = meshes.get("Sidewalk_Straight_3m")
    yaw = _axis_yaw(sidewalk, "y")
    size = base._mesh_size(sidewalk)
    step = max(260.0, max(size.x, size.y) * 0.96)
    for side in (-1, 1):
        x = side * 720.0
        for index in range(-12, 13):
            y = index * step
            # Keep the central junction visually open.
            if abs(y) < step * 0.55:
                continue
            mesh_name = "Sidewalk_Planter" if index in (-8, 7) else "Sidewalk_Straight_3m"
            actor = base._spawn(
                actors, meshes, mats, mesh_name,
                f"Sidewalk_{'W' if side < 0 else 'E'}_{index:+03d}",
                (x, y, 4.0), yaw=yaw, material=mats.get("concrete"),
                collision=True,
            )
            spawned += 1 if actor else 0

    for side, yaw_corner in ((-1, 0.0), (1, 180.0)):
        for y in (-3550.0, 3550.0):
            actor = base._spawn(
                actors, meshes, mats, "Sidewalk_Corner_Flat_3m",
                f"SidewalkCorner_{side}_{int(y)}", (side * 720.0, y, 4.0),
                yaw=yaw_corner, material=mats.get("concrete"),
            )
            spawned += 1 if actor else 0
    return spawned


def _decorate_workshop(actors, meshes, mats):
    spawned = 0
    cx, cy = 1550.0, 950.0
    wall_module, level_h = base._wall_dimensions(meshes.get("Metal_FirstFloor_Wall"))
    half_w, half_d = wall_module * 3.0, wall_module * 2.5
    cornice_z = 18.0 + level_h - 36.0

    spawned += _line(actors, meshes, mats, "Cornice_Metal_Center", "WorkshopCorniceS",
                     (cx - half_w, cy - half_d - 6.0), (cx + half_w, cy - half_d - 6.0), cornice_z,
                     material=mats.get("metal"), collision=False)
    spawned += _line(actors, meshes, mats, "Cornice_Metal_Center", "WorkshopCorniceN",
                     (cx - half_w, cy + half_d + 6.0), (cx + half_w, cy + half_d + 6.0), cornice_z,
                     material=mats.get("metal"), collision=False)

    for index, (x, y) in enumerate((
        (cx - half_w, cy - half_d), (cx + half_w, cy - half_d),
        (cx - half_w, cy + half_d), (cx + half_w, cy + half_d),
    )):
        for part, z in (("Metal_Column_Bottom", 18.0), ("Metal_Column_Center", 18.0 + level_h * 0.34), ("Metal_Column_Top", 18.0 + level_h * 0.72)):
            actor = base._spawn(actors, meshes, mats, part, f"WorkshopCorner_{index}_{part}", (x, y, z), material=mats.get("metal"))
            spawned += 1 if actor else 0

    # Real shop-floor rhythm instead of one empty rectangular room.
    for index, x in enumerate((cx - 540.0, cx, cx + 540.0)):
        actor = base._spawn(actors, meshes, mats, "Metal_FullWindow", f"WorkshopNorthWindow_{index}",
                            (x, cy + half_d + 3.0, 18.0), yaw=180.0, material=mats.get("metal"))
        spawned += 1 if actor else 0
    for index, x in enumerate((cx - 510.0, cx + 510.0)):
        actor = base._spawn(actors, meshes, mats, "Floor_Inset", f"WorkshopBayFloor_{index}",
                            (x, cy + 100.0, 21.0), material=mats.get("concrete"), collision=True)
        spawned += 1 if actor else 0
    return spawned


def _decorate_store(actors, meshes, mats):
    spawned = 0
    cx, cy = -1480.0, 1250.0
    module, level_h = base._wall_dimensions(meshes.get("Brick_Plain_1"))
    half = module * 2.0
    top_z = 18.0 + level_h - 24.0

    for face, start, end in (
        ("S", (cx - half, cy - half - 7.0), (cx + half, cy - half - 7.0)),
        ("N", (cx - half, cy + half + 7.0), (cx + half, cy + half + 7.0)),
    ):
        spawned += _line(actors, meshes, mats, "Cornice_Brick_Center", f"StoreCornice{face}", start, end, top_z,
                         material=mats.get("brick"), collision=False)

    for index, (x, y) in enumerate(((cx - half, cy - half), (cx + half, cy - half), (cx - half, cy + half), (cx + half, cy + half))):
        actor = base._spawn(actors, meshes, mats, "Brick_CornerColumn_Bottom", f"StoreCorner_{index}",
                            (x, y, 18.0), material=mats.get("brick"))
        spawned += 1 if actor else 0

    # A stronger readable storefront on the road-facing side.
    for index, x in enumerate((cx - module * 1.25, cx + module * 1.25)):
        actor = base._spawn(actors, meshes, mats, "Brick_RedWhite_DoubleWindow", f"StoreFrontWindow_{index}",
                            (x, cy - half - 5.0, 18.0), material=mats.get("brick"))
        spawned += 1 if actor else 0
    actor = base._spawn(actors, meshes, mats, "DoorFrame_Trim", "StoreFrontDoorFrame",
                        (cx, cy - half - 10.0, 18.0), material=mats.get("trim"), collision=False)
    spawned += 1 if actor else 0
    return spawned


def _decorate_motel(actors, meshes, mats):
    spawned = 0
    cx, cy = -1700.0, -1450.0
    module, level_h = base._wall_dimensions(meshes.get("Brick_Plain_1"))
    half_w, half_d = module * 3.0, module * 2.0
    upper_z = 18.0 + level_h
    roof_z = 18.0 + level_h * 2.0

    spawned += _line(actors, meshes, mats, "Brick_TopTrim", "MotelBeltS",
                     (cx - half_w, cy - half_d - 7.0), (cx + half_w, cy - half_d - 7.0), upper_z - 28.0,
                     material=mats.get("brick"), collision=False)
    spawned += _line(actors, meshes, mats, "Roof_SlateCornice_Center", "MotelRooflineS",
                     (cx - half_w, cy - half_d - 10.0), (cx + half_w, cy - half_d - 10.0), roof_z,
                     material=mats.get("roof"), collision=False)

    for index, x in enumerate((cx - module * 2.0, cx, cx + module * 2.0)):
        actor = base._spawn(actors, meshes, mats, "Roof_SlateCornice_Window_1", f"MotelDormer_{index}",
                            (x, cy - half_d - 14.0, roof_z + 6.0), material=mats.get("roof"), collision=False)
        spawned += 1 if actor else 0
    for index, x in enumerate((cx - module * 2.35, cx - module * 0.8, cx + module * 0.8, cx + module * 2.35)):
        actor = base._spawn(actors, meshes, mats, "Brick_Window_CurvedDouble", f"MotelUpperWindow_{index}",
                            (x, cy - half_d - 5.0, upper_z), material=mats.get("brick"))
        spawned += 1 if actor else 0
    return spawned


def _build_substation_detail(actors, meshes, mats):
    spawned = 0
    # A coherent service yard: two equipment bays, perimeter posts and a small
    # control shed. No random roadside prop soup.
    for row, y in enumerate((-2370.0, -2020.0, -1670.0)):
        for col, x in enumerate((1100.0, 1500.0, 1900.0, 2300.0)):
            mesh_name = "Prop_ACUnit" if (row + col) % 2 == 0 else "Metal_Column_Small_Bottom"
            actor = base._spawn(actors, meshes, mats, mesh_name, f"SubstationGrid_{row}_{col}",
                                (x, y, 8.0), material=mats.get("metal"))
            spawned += 1 if actor else 0

    shed_center = (2680.0, -2050.0)
    for index, (x, y, yaw) in enumerate((
        (shed_center[0] - 300.0, shed_center[1], 90.0),
        (shed_center[0] + 300.0, shed_center[1], -90.0),
        (shed_center[0], shed_center[1] + 300.0, 180.0),
    )):
        actor = base._spawn(actors, meshes, mats, "Metal_Plain_3", f"SubstationShedWall_{index}",
                            (x, y, 8.0), yaw=yaw, material=mats.get("metal"))
        spawned += 1 if actor else 0
    actor = base._spawn(actors, meshes, mats, "Metal_FullWindow", "SubstationShedFront",
                        (shed_center[0], shed_center[1] - 300.0, 8.0), material=mats.get("metal"))
    spawned += 1 if actor else 0
    actor = base._spawn(actors, meshes, mats, "Roof_2x2", "SubstationShedRoof",
                        (shed_center[0], shed_center[1], 330.0), material=mats.get("roof"))
    spawned += 1 if actor else 0
    return spawned


def _build_midground(actors, meshes, mats):
    spawned = 0
    placements = [
        ("Building_Small_1", (-3300.0, 3150.0, 0.0), 14.0, (0.92, 0.92, 0.92)),
        ("Building_Medium_2_001", (3300.0, 3400.0, 0.0), -9.0, (0.86, 0.86, 0.86)),
        ("Building_Small_1", (-3550.0, 250.0, 0.0), -20.0, (0.88, 0.88, 0.88)),
        ("Building_Medium_2_001", (3550.0, 100.0, 0.0), 18.0, (0.90, 0.90, 0.90)),
        ("Building_Small_1", (3250.0, -3950.0, 0.0), -12.0, (0.86, 0.86, 0.86)),
        ("Building_Medium_2_001", (-3400.0, -4200.0, 0.0), 11.0, (0.92, 0.92, 0.92)),
    ]
    for index, (mesh_name, location, yaw, scale) in enumerate(placements):
        actor = base._spawn(actors, meshes, mats, mesh_name, f"Midground_{index}", location,
                            yaw=yaw, scale=scale, material=mats.get("concrete"))
        spawned += 1 if actor else 0
    return spawned


def _stair_run(actors, meshes, mats, key, start, end, modules):
    sx, sy, sz = start
    ex, ey, ez = end
    spawned = 0
    for index in range(modules):
        t = (index + 0.5) / modules
        x = sx + (ex - sx) * t
        y = sy + (ey - sy) * t
        z = sz + (ez - sz) * t
        actor = base._spawn(actors, meshes, mats, "Stairs_Entrance_Concrete", f"{key}_Step_{index:02d}",
                            (x, y, z), yaw=180.0, material=mats.get("concrete"), collision=True)
        spawned += 1 if actor else 0
        if index % 2 == 0:
            for side in (-1, 1):
                rail = base._spawn(actors, meshes, mats, "Stairs_Rails_Metal_Straight_2",
                                   f"{key}_Rail_{index:02d}_{side}", (x + side * 300.0, y, z + 18.0),
                                   yaw=180.0, material=mats.get("metal"), collision=False)
                spawned += 1 if rail else 0
    return spawned


def _build_modular_underground_access(actors, meshes, mats):
    spawned = 0
    # Replace every legacy cube stair with pack-native stair modules. The two
    # runs intentionally overlap their endpoints so CharacterMovement never has
    # to jump a decorative gap.
    spawned += _stair_run(actors, meshes, mats, "MetroDescentA",
                          (0.0, -4370.0, -15.0), (0.0, -6800.0, -790.0), 8)
    actor = base._spawn(actors, meshes, mats, "Floor_4x4", "MetroLandingA",
                        (0.0, -7000.0, -805.0), scale=(1.2, 1.0, 1.0), material=mats.get("concrete"))
    spawned += 1 if actor else 0

    spawned += _stair_run(actors, meshes, mats, "MetroDescentB",
                          (0.0, -8450.0, -855.0), (0.0, -9600.0, -1235.0), 4)
    actor = base._spawn(actors, meshes, mats, "Floor_4x4", "BreachLanding",
                        (0.0, -9750.0, -1250.0), scale=(1.0, 0.85, 1.0), material=mats.get("concrete"))
    spawned += 1 if actor else 0
    return spawned


def _stage_extra_loot(actors):
    path = base._ensure_loot_blueprint()
    placements = [
        ("WorkshopServiceCache", (2350.0, 760.0, 24.0), 2, 2711, "Service Parts Crate"),
        ("StoreBackroomFood", (-1190.0, 1570.0, 24.0), 1, 1721, "Backroom Supplies"),
        ("MotelOfficeLockbox", (-1470.0, -1760.0, 24.0), 2, 3821, "Motel Office Lockbox"),
        ("SubstationEmergency", (2730.0, -2050.0, 24.0), 3, 4931, "Emergency Grid Cache"),
        ("MetroPlatformCache", (470.0, -7050.0, -800.0), 3, 5841, "Platform Maintenance Cache"),
    ]
    spawned = 0
    for args in placements:
        actor = base._spawn_loot(actors, path, *args)
        spawned += 1 if actor else 0
    return spawned


def apply_all():
    if not has_source_pack():
        rw.warn(f"CityBuildings source pack not found at {base.SOURCE_FBX}")
        return False
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return False

    _extend_selection()
    meshes, textures = base.import_city_assets()
    mats = base.ensure_city_materials(textures)

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    removed = _cleanup_authoring_layers(actors)

    # Start from the already proven authored district, then make it read as an
    # actual place rather than a greybox with prettier textures.
    base._build_roads(actors, meshes, mats)
    base._build_district(actors, meshes, mats)
    base._build_subway_headhouse(actors, meshes, mats)
    base._stage_loot(actors)

    rich_count = 0
    rich_count += _build_sidewalks(actors, meshes, mats)
    rich_count += _decorate_workshop(actors, meshes, mats)
    rich_count += _decorate_store(actors, meshes, mats)
    rich_count += _decorate_motel(actors, meshes, mats)
    rich_count += _build_substation_detail(actors, meshes, mats)
    rich_count += _build_midground(actors, meshes, mats)
    rich_count += _build_modular_underground_access(actors, meshes, mats)
    loot_count = _stage_extra_loot(actors)

    level.save_current_level()
    try:
        rw.asset_library.save_directory(base.CITY_ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass

    rw.log(
        f"CITY MAP V2 complete: removed {removed} stale authored actors; "
        f"added {rich_count} coherent CityKit structural/detail actors and {loot_count} extra loot containers"
    )
    return True


if __name__ == "__main__":
    apply_all()
