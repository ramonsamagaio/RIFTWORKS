from __future__ import annotations

import math
import unreal

import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_REPAIR_"
CITY_MESH = "/Game/Riftworks/CityKit/Meshes"
CITY_MAT = "/Game/Riftworks/CityKit/Materials"


def _component(actor, component_class):
    if not actor:
        return None
    try:
        return actor.get_component_by_class(component_class)
    except Exception:
        return None


def _label(actor, name):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _load(path):
    try:
        return unreal.load_asset(path)
    except Exception:
        return None


def _destroy_previous(actors):
    removed = 0
    bad_stair_prefixes = (
        "RIFT_CITY_MetroDescentA_",
        "RIFT_CITY_MetroDescentB_",
        "RIFT_CITY_MetroLandingA",
        "RIFT_CITY_BreachLanding",
    )
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        if label.startswith(PREFIX) or label.startswith(bad_stair_prefixes):
            try:
                actors.destroy_actor(actor)
                removed += 1
            except Exception:
                pass
    return removed


def _spawn_static(actors, name, mesh, location, material=None, scale=(1.0, 1.0, 1.0), collision=True, hidden=False):
    if not mesh:
        return None
    try:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), rw.rotator())
    except Exception:
        return None
    _label(actor, name)
    if not actor:
        return None

    actor.set_actor_scale3d(unreal.Vector(*scale))
    comp = _component(actor, unreal.StaticMeshComponent)
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
        if material:
            try:
                count = max(1, comp.get_num_materials())
            except Exception:
                count = 1
            for slot in range(count):
                try:
                    comp.set_material(slot, material)
                except Exception:
                    pass
        if hidden:
            try:
                comp.set_hidden_in_game(True)
            except Exception:
                pass
    if hidden:
        try:
            actor.set_actor_hidden_in_game(True)
        except Exception:
            pass
    try:
        actor.set_actor_enable_collision(collision)
    except Exception:
        pass
    return actor


def _spawn_cube(actors, name, center, size_cm, material=None, collision=True, hidden=False):
    cube = _load("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        return None
    scale = (size_cm[0] / 100.0, size_cm[1] / 100.0, size_cm[2] / 100.0)
    return _spawn_static(actors, name, cube, center, material, scale, collision, hidden)


def _spawn_foundation(actors):
    mesh = _load(f"{CITY_MESH}/Street_Asphalt_9x9")
    material = _load(f"{CITY_MAT}/M_City_Asphalt_PBR2")
    if not mesh:
        rw.warn("Runtime repair could not find CityKit Street_Asphalt_9x9")
        return 0

    try:
        bounds = mesh.get_bounds()
        size_x = max(650.0, float(bounds.box_extent.x) * 2.0)
        size_y = max(650.0, float(bounds.box_extent.y) * 2.0)
        bottom = float(bounds.origin.z - bounds.box_extent.z)
    except Exception:
        size_x = size_y = 900.0
        bottom = 0.0

    step_x = size_x * 0.965
    step_y = size_y * 0.965
    min_x, max_x = -4700.0, 4700.0
    min_y, max_y = -4700.0, 4700.0
    nx = int(math.ceil((max_x - min_x) / step_x)) + 1
    ny = int(math.ceil((max_y - min_y) / step_y)) + 1
    origin_x = (min_x + max_x) * 0.5 - (nx - 1) * step_x * 0.5
    origin_y = (min_y + max_y) * 0.5 - (ny - 1) * step_y * 0.5

    spawned = 0
    for ix in range(nx):
        for iy in range(ny):
            x = origin_x + ix * step_x
            y = origin_y + iy * step_y
            # The pack road geometry remains a few centimeters above this layer.
            # Overlap is intentional: no seams, no black void, no falling between tiles.
            actor = _spawn_static(
                actors,
                f"Foundation_{ix:02d}_{iy:02d}",
                mesh,
                (x, y, -10.0 - bottom),
                material,
                collision=True,
            )
            spawned += 1 if actor else 0
    return spawned


def _spawn_fail_safe_collision(actors):
    # These are invisible, collision-only catch floors. They do not replace the
    # visible CityKit surfaces. Their job is to make an accidental missing mesh
    # a recoverable bug instead of an infinite fall that destroys a playtest.
    _spawn_cube(actors, "SafetyFloor_Surface", (0.0, -300.0, -145.0), (13000.0, 13500.0, 70.0), None, True, True)
    _spawn_cube(actors, "SafetyFloor_Station", (0.0, -7000.0, -1015.0), (3600.0, 3700.0, 70.0), None, True, True)
    _spawn_cube(actors, "SafetyFloor_Breach", (0.0, -10000.0, -1470.0), (4200.0, 3800.0, 70.0), None, True, True)


def _rebuild_stairs(actors):
    concrete = _load(f"{CITY_MAT}/M_City_Concrete_PBR2")
    metal = _load(f"{CITY_MAT}/M_City_Metal_PBR2")

    # Surface -> station. 31 cm rise is below the 45 cm Character step limit.
    for i in range(29):
        y = -4400.0 - i * 78.0
        top_z = -8.0 - i * 31.0
        _spawn_cube(actors, f"SurfaceStair_{i:02d}", (0.0, y, top_z - 11.0), (760.0, 92.0, 22.0), concrete, True)
        if i % 4 == 0:
            for side in (-1, 1):
                _spawn_cube(actors, f"SurfaceRailPost_{i:02d}_{side}", (side * 370.0, y, top_z + 70.0), (18.0, 22.0, 165.0), metal, True)

    _spawn_cube(actors, "StationLanding", (0.0, -6715.0, -920.0), (820.0, 430.0, 24.0), concrete, True)

    # Station -> Breach. Same safe step height and a real connector landing.
    for i in range(14):
        y = -8200.0 - i * 78.0
        top_z = -908.0 - i * 31.0
        _spawn_cube(actors, f"DeepStair_{i:02d}", (0.0, y, top_z - 11.0), (760.0, 92.0, 22.0), concrete, True)
        if i % 4 == 0:
            for side in (-1, 1):
                _spawn_cube(actors, f"DeepRailPost_{i:02d}_{side}", (side * 370.0, y, top_z + 70.0), (18.0, 22.0, 165.0), metal, True)

    _spawn_cube(actors, "BreachConnector", (0.0, -9480.0, -1329.0), (820.0, 1120.0, 24.0), concrete, True)


def _spawn_nav_bounds(actors):
    nav_class = getattr(unreal, "NavMeshBoundsVolume", None)
    if nav_class is None:
        rw.warn("NavMeshBoundsVolume Python class unavailable; deterministic NPC locomotion remains active")
        return None
    try:
        nav = actors.spawn_actor_from_class(nav_class, unreal.Vector(0.0, -3000.0, -500.0), rw.rotator())
        _label(nav, "NavBounds")
        # Default brush is intentionally scaled very large so surface, station and
        # Breach floors all contribute to the same dynamic Recast dataset.
        nav.set_actor_scale3d(unreal.Vector(65.0, 90.0, 22.0))
        return nav
    except Exception as exc:
        rw.warn(f"Could not spawn NavMeshBoundsVolume: {exc}")
        return None


def _spawn_point_light(actors, name, location, color, intensity=520.0, radius=1450.0):
    try:
        actor = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*location), rw.rotator())
        _label(actor, name)
        comp = _component(actor, unreal.PointLightComponent)
        if comp:
            rw.safe_set(comp, "intensity_units", unreal.LightUnits.LUMENS)
            rw.safe_set(comp, "intensity", intensity)
            rw.safe_set(comp, "attenuation_radius", radius)
            rw.safe_set(comp, "light_color", unreal.Color(*color, 255))
            rw.safe_set(comp, "cast_shadows", False)
            rw.safe_set(comp, "volumetric_scattering_intensity", 0.015)
        return actor
    except Exception:
        return None


def _repair_lighting(actors):
    # Kill the red realtime-capture warning and make the city readable without
    # turning the player's flashlight into a texture-destroying white disk.
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""

        sky = _component(actor, unreal.SkyLightComponent)
        if sky:
            rw.safe_set(sky, "real_time_capture", False)
            rw.safe_set(sky, "intensity", 0.22)
            try:
                sky.recapture_sky()
            except Exception:
                pass

        directional = _component(actor, unreal.DirectionalLightComponent)
        if directional and "Moon" in label:
            rw.safe_set(directional, "intensity", 0.18)
            rw.safe_set(directional, "volumetric_scattering_intensity", 0.025)
            rw.safe_set(directional, "atmosphere_sun_light", True)

    sky_atmosphere_class = getattr(unreal, "SkyAtmosphere", None)
    if sky_atmosphere_class:
        try:
            atmosphere = actors.spawn_actor_from_class(sky_atmosphere_class, unreal.Vector(0.0, 0.0, 0.0), rw.rotator())
            _label(atmosphere, "SkyAtmosphere")
            component_class = getattr(unreal, "SkyAtmosphereComponent", None)
            comp = _component(atmosphere, component_class) if component_class else None
            if comp:
                rw.safe_set(comp, "rayleigh_scattering_scale", 0.22)
                rw.safe_set(comp, "mie_scattering_scale", 0.05)
        except Exception:
            pass

    # Sparse practical pools. The prior V2 cleanup removed the legacy streetlight
    # actors, leaving most of the city pitch-black outside the flashlight cone.
    light_specs = [
        (-670.0, -3100.0, 390.0, (255, 181, 108)),
        (670.0, -2150.0, 390.0, (150, 185, 255)),
        (-670.0, -1100.0, 390.0, (255, 181, 108)),
        (670.0, 0.0, 390.0, (150, 185, 255)),
        (-670.0, 1150.0, 390.0, (255, 181, 108)),
        (670.0, 2250.0, 390.0, (150, 185, 255)),
        (-670.0, 3300.0, 390.0, (255, 181, 108)),
        (-1480.0, 700.0, 280.0, (255, 166, 92)),
        (1550.0, 120.0, 320.0, (255, 174, 100)),
        (2100.0, -2050.0, 330.0, (122, 176, 255)),
        (0.0, -4150.0, 300.0, (255, 142, 76)),
    ]
    for index, (x, y, z, color) in enumerate(light_specs):
        _spawn_point_light(actors, f"Practical_{index:02d}", (x, y, z), color)


def _mark_skeletal_material(material):
    if not material:
        return
    rw.safe_set(material, "used_with_skeletal_mesh", True)
    try:
        unreal.MaterialEditingLibrary.recompile_material(material)
    except Exception:
        pass
    try:
        rw.asset_library.save_asset(material.get_path_name().split(".")[0], only_if_is_dirty=False)
    except Exception:
        pass


def _repair_character_materials(actors):
    humanoid_mat = rv.create_surface_material(
        "M_Humanoid_Scavenger",
        (0.105, 0.050, 0.012),
        roughness=0.86,
        metallic=0.02,
        emissive=None,
        variation=0.09,
    )
    colossus_mat = _load("/Game/Riftworks/Materials/World/M_Colossus_Graphite")
    _mark_skeletal_material(humanoid_mat)
    _mark_skeletal_material(colossus_mat)

    cdo = rw.blueprint_cdo(f"{rw.BP_DIR}/BP_RiftAnimatedHumanoid")
    if cdo and humanoid_mat:
        try:
            mesh = cdo.get_editor_property("mesh")
            mesh.set_material(0, humanoid_mat)
            rw.asset_library.save_asset(f"{rw.BP_DIR}/BP_RiftAnimatedHumanoid", only_if_is_dirty=False)
        except Exception:
            pass

    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        mesh = _component(actor, unreal.SkeletalMeshComponent)
        if not mesh:
            continue
        if label.startswith("RIFT_AUTO_Humanoid_") and humanoid_mat:
            try:
                mesh.set_material(0, humanoid_mat)
            except Exception:
                pass
        elif "Colossus" in label and colossus_mat:
            try:
                mesh.set_material(0, colossus_mat)
            except Exception:
                pass


def _move_actor(actor, location):
    if not actor:
        return
    try:
        actor.set_actor_location(unreal.Vector(*location), False, False)
    except Exception:
        pass


def _restage_enemies(actors):
    by_label = {}
    for actor in list(actors.get_all_level_actors()):
        try:
            by_label[actor.get_actor_label()] = actor
        except Exception:
            pass

    # Surface humans are placed on guaranteed foundation, not on decorative
    # building footprints. Z is capsule-center height for a 92 cm half-height.
    placements = {
        "RIFT_AUTO_Humanoid_00": (0.0, 2750.0, 95.0),
        "RIFT_AUTO_Humanoid_01": (-240.0, -1550.0, 95.0),
        "RIFT_AUTO_Humanoid_02": (260.0, -3000.0, 95.0),
        "RIFT_AUTO_Humanoid_03": (-300.0, -7200.0, -815.0),
        "RIFT_AUTO_Humanoid_04": (420.0, -10200.0, -1225.0),
        "RIFT_EXTRA_BreachGolem_00": (0.0, -7480.0, -815.0),
        "RIFT_EXTRA_BreachGolem_01": (-350.0, -10300.0, -1225.0),
    }
    for label, location in placements.items():
        _move_actor(by_label.get(label), location)


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return False

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    removed = _destroy_previous(actors)
    foundation = _spawn_foundation(actors)
    _spawn_fail_safe_collision(actors)
    _rebuild_stairs(actors)
    nav = _spawn_nav_bounds(actors)
    _repair_lighting(actors)
    _repair_character_materials(actors)
    _restage_enemies(actors)

    level.save_current_level()

    if nav:
        try:
            unreal.SystemLibrary.execute_console_command(nav, "RebuildNavigation")
        except Exception:
            pass

    rw.log(
        f"RUNTIME REPAIR complete: removed={removed}, foundation_tiles={foundation}, "
        "continuous collision + safe stairs + nav bounds + readable night lighting + grounded enemy staging"
    )
    return True


if __name__ == "__main__":
    apply_all()
