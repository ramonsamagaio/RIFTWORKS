from __future__ import annotations

import math
import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_ART_"


def _mesh(path: str):
    try:
        return unreal.load_asset(path)
    except Exception:
        return None


def _component(actor, component_class):
    if not actor:
        return None
    try:
        return actor.get_component_by_class(component_class)
    except Exception:
        return None


def _label(actor, name: str):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _spawn_mesh(actors, name: str, mesh, location, size_cm, material=None, rotation=None):
    if not mesh:
        return None
    rotation = rotation or unreal.Rotator()
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), rotation)
    _label(actor, name)
    if not actor:
        return None
    comp = _component(actor, unreal.StaticMeshComponent)
    if comp:
        try:
            comp.set_static_mesh(mesh)
        except Exception:
            rw.safe_set(comp, "static_mesh", mesh)
        if material:
            try:
                comp.set_material(0, material)
            except Exception:
                pass
        try:
            comp.set_collision_profile_name("BlockAll")
        except Exception:
            pass
    actor.set_actor_scale3d(unreal.Vector(size_cm[0] / 100.0, size_cm[1] / 100.0, size_cm[2] / 100.0))
    return actor


def _spawn_cube(actors, name, location, size_cm, material=None, rotation=None):
    return _spawn_mesh(actors, name, _mesh("/Engine/BasicShapes/Cube.Cube"), location, size_cm, material, rotation)


def _spawn_cylinder(actors, name, location, diameter_cm, height_cm, material=None, rotation=None):
    # Engine cylinder is 100 cm high and 100 cm diameter.
    return _spawn_mesh(actors, name, _mesh("/Engine/BasicShapes/Cylinder.Cylinder"), location,
                       (diameter_cm, diameter_cm, height_cm), material, rotation)


def _spawn_cone(actors, name, location, diameter_cm, height_cm, material=None, rotation=None):
    return _spawn_mesh(actors, name, _mesh("/Engine/BasicShapes/Cone.Cone"), location,
                       (diameter_cm, diameter_cm, height_cm), material, rotation)


def _spawn_point_light(actors, name: str, location, color, intensity=900.0, radius=1000.0, shadows=False):
    try:
        light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*location), unreal.Rotator())
        _label(light, name)
        comp = _component(light, unreal.PointLightComponent)
        if comp:
            rw.safe_set(comp, "intensity_units", unreal.LightUnits.LUMENS)
            rw.safe_set(comp, "intensity", intensity)
            rw.safe_set(comp, "attenuation_radius", radius)
            rw.safe_set(comp, "light_color", unreal.Color(int(color[0]), int(color[1]), int(color[2]), 255))
            rw.safe_set(comp, "cast_shadows", shadows)
            rw.safe_set(comp, "volumetric_scattering_intensity", 0.08)
        return light
    except Exception as exc:
        rw.warn(f"Point light {name} skipped: {exc}")
        return None


def _spawn_shell(actors, name: str, center, width, depth, height, mats, door_width=210.0,
                 garage=False, two_story=False, window_count=3):
    x, y, z = center
    wall = 24.0
    concrete = mats["concrete"]
    metal = mats["metal"]
    glass = mats["glass"]
    asphalt = mats["asphalt"]

    _spawn_cube(actors, f"{name}_Floor", (x, y, z + 12), (width, depth, 24), asphalt)
    _spawn_cube(actors, f"{name}_Rear", (x, y + depth * 0.5, z + height * 0.5), (width, wall, height), concrete)
    _spawn_cube(actors, f"{name}_Left", (x - width * 0.5, y, z + height * 0.5), (wall, depth, height), concrete)
    _spawn_cube(actors, f"{name}_Right", (x + width * 0.5, y, z + height * 0.5), (wall, depth, height), concrete)

    front_y = y - depth * 0.5
    opening = min(door_width, width * 0.62)
    side = (width - opening) * 0.5
    _spawn_cube(actors, f"{name}_FrontL", (x - (opening + side) * 0.5, front_y, z + height * 0.5), (side, wall, height), concrete)
    _spawn_cube(actors, f"{name}_FrontR", (x + (opening + side) * 0.5, front_y, z + height * 0.5), (side, wall, height), concrete)
    _spawn_cube(actors, f"{name}_Header", (x, front_y, z + height - 35), (opening, wall, 70), metal)
    _spawn_cube(actors, f"{name}_Roof", (x, y, z + height + 14), (width + 50, depth + 50, 28), metal)

    # Dark glass panels read as windows under the flashlight without requiring a detailed texture set yet.
    for i in range(max(1, window_count)):
        alpha = (i + 1) / float(window_count + 1)
        wx = x - width * 0.40 + alpha * width * 0.80
        if abs(wx - x) < opening * 0.56:
            continue
        _spawn_cube(actors, f"{name}_Glass_{i}", (wx, front_y - 13, z + height * 0.58),
                    (min(210, width / (window_count + 1) * 0.62), 8, 125), glass)

    # Structural ribs break the flat facade silhouette.
    for i in range(1, 4):
        px = x - width * 0.5 + width * (i / 4.0)
        _spawn_cube(actors, f"{name}_Rib_{i}", (px, y + depth * 0.5 + 14, z + height * 0.52), (18, 34, height * 0.84), metal)

    if garage:
        canopy_width = min(width * 0.75, 1100)
        _spawn_cube(actors, f"{name}_Canopy", (x, front_y - 180, z + height * 0.78), (canopy_width, 360, 22), metal)
        for side_sign in (-1, 1):
            _spawn_cube(actors, f"{name}_CanopyPost_{side_sign}",
                        (x + side_sign * canopy_width * 0.43, front_y - 245, z + height * 0.38),
                        (26, 26, height * 0.76), metal)
        # Workshop benches / machine blocks.
        for j in range(3):
            _spawn_cube(actors, f"{name}_Bench_{j}", (x - width * 0.30 + j * width * 0.30, y + depth * 0.18, z + 60),
                        (260, 85, 120), mats["rust"] if j == 1 else metal)

    if two_story:
        mid = z + height * 0.50
        _spawn_cube(actors, f"{name}_MidFloor", (x, y, mid), (width - 40, depth - 40, 20), concrete)
        # Exterior balcony and simple staircase.
        _spawn_cube(actors, f"{name}_Balcony", (x, front_y - 120, mid + 10), (width * 0.78, 230, 20), metal)
        for s in range(7):
            _spawn_cube(actors, f"{name}_Step_{s}",
                        (x + width * 0.36, front_y - 260 - s * 55, z + 35 + s * 55), (220, 105, 20), metal)


def _spawn_car(actors, name, location, yaw, mats, scale=1.0):
    x, y, z = location
    rot = unreal.Rotator(0.0, yaw, 0.0)
    body_mat = mats["rust"] if int(abs(x + y)) % 2 else mats["metal"]
    _spawn_cube(actors, f"{name}_Body", (x, y, z + 48 * scale), (390 * scale, 175 * scale, 62 * scale), body_mat, rot)
    _spawn_cube(actors, f"{name}_Cabin", (x - 22 * scale, y, z + 92 * scale), (205 * scale, 160 * scale, 62 * scale), mats["glass"], rot)
    wheel_offsets = [(125, 88), (125, -88), (-125, 88), (-125, -88)]
    yaw_rad = math.radians(yaw)
    for idx, (ox, oy) in enumerate(wheel_offsets):
        rx = ox * math.cos(yaw_rad) - oy * math.sin(yaw_rad)
        ry = ox * math.sin(yaw_rad) + oy * math.cos(yaw_rad)
        _spawn_cylinder(actors, f"{name}_Wheel_{idx}", (x + rx * scale, y + ry * scale, z + 34 * scale),
                        64 * scale, 34 * scale, mats["rubber"], unreal.Rotator(90.0, yaw, 0.0))


def _spawn_tree(actors, name, location, mats, scale=1.0):
    x, y, z = location
    _spawn_cylinder(actors, f"{name}_Trunk", (x, y, z + 160 * scale), 48 * scale, 320 * scale, mats["trunk"])
    _spawn_cone(actors, f"{name}_CrownA", (x, y, z + 370 * scale), 250 * scale, 360 * scale, mats["foliage"])
    _spawn_cone(actors, f"{name}_CrownB", (x + 28 * scale, y - 20 * scale, z + 500 * scale), 190 * scale, 270 * scale, mats["foliage"])


def _spawn_streetlight(actors, name, location, mats, lit=True, warm=True):
    x, y, z = location
    _spawn_cylinder(actors, f"{name}_Pole", (x, y, z + 220), 24, 440, mats["metal"])
    _spawn_cube(actors, f"{name}_Arm", (x + 55, y, z + 430), (130, 18, 18), mats["metal"])
    _spawn_cube(actors, f"{name}_Lamp", (x + 115, y, z + 418), (50, 36, 22), mats["lamp"] if lit else mats["metal"])
    if lit:
        color = (255, 190, 105) if warm else (150, 190, 255)
        _spawn_point_light(actors, f"{name}_Light", (x + 112, y, z + 390), color, 820.0, 930.0, False)


def _spawn_surface(actors, mats):
    # Ground is split around the central corridor so the underground stairwell can physically pierce the surface.
    _spawn_cube(actors, "GroundWest", (-3300, 0, -55), (5400, 12000, 110), mats["ground"])
    _spawn_cube(actors, "GroundEast", (3300, 0, -55), (5400, 12000, 110), mats["ground"])
    _spawn_cube(actors, "GroundNorthCenter", (0, 800, -55), (1200, 9600, 110), mats["ground"])

    _spawn_cube(actors, "RoadMain", (0, 550, 2), (920, 9000, 12), mats["asphalt"])
    _spawn_cube(actors, "SidewalkL", (-570, 550, 12), (210, 9000, 24), mats["concrete"])
    _spawn_cube(actors, "SidewalkR", (570, 550, 12), (210, 9000, 24), mats["concrete"])

    # Road paint, broken and sparse rather than pristine.
    for i in range(17):
        y = -3300 + i * 470
        if i % 3 != 1:
            _spawn_cube(actors, f"RoadDash_{i}", (0, y, 12), (16, 190, 4), mats["road_yellow"])
    for side in (-1, 1):
        for i in range(9):
            y = -3100 + i * 900
            _spawn_cube(actors, f"RoadEdge_{side}_{i}", (side * 415, y, 11), (9, 620, 3), mats["road_white"])

    _spawn_shell(actors, "Workshop", (1550, 950, 0), 1900, 1600, 650, mats, door_width=760, garage=True, window_count=4)
    _spawn_shell(actors, "CornerStore", (-1480, 1250, 0), 1380, 1040, 430, mats, door_width=220, garage=False, window_count=5)
    _spawn_shell(actors, "Motel", (-1700, -1450, 0), 1850, 1420, 820, mats, door_width=250, two_story=True, window_count=5)

    # Substation / utility yard.
    sx, sy = 1650, -1750
    _spawn_cube(actors, "SubstationPad", (sx, sy, 8), (2000, 1500, 16), mats["concrete_dark"])
    for i, px in enumerate((-520, 0, 520)):
        _spawn_cube(actors, f"Transformer_{i}", (sx + px, sy, 90), (330, 250, 180), mats["metal"])
        _spawn_cube(actors, f"TransformerTop_{i}", (sx + px, sy, 205), (220, 170, 55), mats["rust"])
    # Fence-like perimeter using posts and rails.
    for ix in range(-4, 5):
        px = sx + ix * 235
        for yy in (sy - 720, sy + 720):
            _spawn_cube(actors, f"SubFenceP_{ix}_{yy}", (px, yy, 90), (18, 18, 180), mats["metal"])
    for iy in range(-3, 4):
        py = sy + iy * 235
        for xx in (sx - 960, sx + 960):
            _spawn_cube(actors, f"SubFenceS_{iy}_{xx}", (xx, py, 90), (18, 18, 180), mats["metal"])
    for yy in (sy - 720, sy + 720):
        _spawn_cube(actors, f"SubRailA_{yy}", (sx, yy, 55), (1900, 14, 14), mats["metal"])
        _spawn_cube(actors, f"SubRailB_{yy}", (sx, yy, 125), (1900, 14, 14), mats["metal"])

    # Streetlights, several intentionally dead.
    for i, y in enumerate((-3150, -2050, -850, 350, 1550, 2750, 3900)):
        side = -1 if i % 2 == 0 else 1
        _spawn_streetlight(actors, f"StreetLamp_{i}", (side * 700, y, 0), mats, lit=(i not in (1, 5)), warm=(i % 3 != 0))

    _spawn_car(actors, "CarA", (-160, 2300, 12), 7, mats, 1.0)
    _spawn_car(actors, "CarB", (210, -650, 12), -17, mats, 0.92)
    _spawn_car(actors, "CarC", (-180, -2850, 12), 12, mats, 1.04)

    # Street clutter / silhouette breakers.
    for i in range(10):
        x = -820 if i % 2 == 0 else 840
        y = -3100 + i * 650
        _spawn_cube(actors, f"Barrier_{i}", (x, y, 34), (145, 55, 68), mats["hazard"] if i % 3 == 0 else mats["concrete"])
    for i, pos in enumerate(((1120, 2400), (1360, 2460), (-950, 2600), (930, -2900), (-1080, -3000))):
        _spawn_cube(actors, f"Crate_{i}", (pos[0], pos[1], 55), (105, 105, 110), mats["rust"] if i % 2 else mats["metal"])

    for i, pos in enumerate(((-2700, 3100), (2700, 2800), (-2850, -2600), (2800, -3200), (-2500, 400))):
        _spawn_tree(actors, f"Tree_{i}", (pos[0], pos[1], 0), mats, 0.85 + i * 0.08)


def _spawn_underground(actors, mats):
    # Strong, obvious portal rather than a hidden hole in procedural ground.
    portal_y = -4250
    _spawn_cube(actors, "UG_PortalLeft", (-430, portal_y, 260), (120, 300, 520), mats["concrete_dark"])
    _spawn_cube(actors, "UG_PortalRight", (430, portal_y, 260), (120, 300, 520), mats["concrete_dark"])
    _spawn_cube(actors, "UG_PortalTop", (0, portal_y, 520), (980, 300, 80), mats["metal"])
    _spawn_cube(actors, "UG_PortalSign", (0, portal_y - 165, 455), (430, 18, 80), mats["emergency"])

    # Walkable staircase from surface to depth -900.
    steps = 16
    for i in range(steps):
        y = -4400 - i * 105
        z = -18 - i * 57
        _spawn_cube(actors, f"UG_Step_{i}", (0, y, z), (720, 120, 24), mats["concrete"])
        _spawn_cube(actors, f"UG_WallL_{i}", (-410, y, z + 180), (40, 125, 390), mats["concrete_dark"])
        _spawn_cube(actors, f"UG_WallR_{i}", (410, y, z + 180), (40, 125, 390), mats["concrete_dark"])
        if i % 4 == 0:
            _spawn_cube(actors, f"UG_EmergencyLamp_{i}", (-375, y, z + 285), (22, 28, 34), mats["emergency"])
            _spawn_point_light(actors, f"UG_EmergencyLight_{i}", (-340, y, z + 260), (255, 44, 22), 240, 430, False)

    hall_z = -925
    hall_y = -6900
    # Station/maintenance hall, clearly human architecture.
    _spawn_cube(actors, "UG_HallFloor", (0, hall_y, hall_z), (1800, 2500, 34), mats["concrete_dark"])
    _spawn_cube(actors, "UG_HallWallL", (-900, hall_y, hall_z + 370), (42, 2500, 760), mats["concrete"])
    _spawn_cube(actors, "UG_HallWallR", (900, hall_y, hall_z + 370), (42, 2500, 760), mats["concrete"])
    _spawn_cube(actors, "UG_HallCeiling", (0, hall_y, hall_z + 750), (1800, 2500, 36), mats["metal"])

    for bay in range(5):
        y = hall_y - 900 + bay * 450
        for side in (-1, 1):
            _spawn_cube(actors, f"UG_Column_{bay}_{side}", (side * 720, y, hall_z + 320), (80, 80, 640), mats["metal"])
        _spawn_cube(actors, f"UG_CeilingBeam_{bay}", (0, y, hall_z + 680), (1450, 70, 70), mats["rust"])
        if bay in (1, 3):
            _spawn_cube(actors, f"UG_Lamp_{bay}", (0, y, hall_z + 635), (260, 30, 24), mats["lamp"])
            _spawn_point_light(actors, f"UG_Light_{bay}", (0, y, hall_z + 570), (255, 193, 112), 760, 920, False)

    # Maintenance room and pipes make the station more than a corridor.
    _spawn_shell(actors, "UG_Maintenance", (570, hall_y + 420, hall_z + 18), 560, 720, 360, mats, door_width=170, garage=False, window_count=1)
    for p in range(4):
        _spawn_cylinder(actors, f"UG_Pipe_{p}", (-700 + p * 150, hall_y + 900, hall_z + 590), 55, 900, mats["rust"], unreal.Rotator(90.0, 0.0, 0.0))

    # Second descent and Breach chamber.
    for i in range(10):
        y = -8200 - i * 115
        z = hall_z - 20 - i * 40
        _spawn_cube(actors, f"DeepStep_{i}", (0, y, z), (760, 130, 24), mats["metal"])
        _spawn_cube(actors, f"DeepRailL_{i}", (-390, y, z + 95), (18, 130, 190), mats["rust"])
        _spawn_cube(actors, f"DeepRailR_{i}", (390, y, z + 95), (18, 130, 190), mats["rust"])

    chamber_z = -1340
    chamber_y = -9900
    _spawn_cube(actors, "BreachFloor", (0, chamber_y, chamber_z), (2600, 2300, 45), mats["breach_dark"])
    for side in (-1, 1):
        _spawn_cube(actors, f"BreachWall_{side}", (side * 1300, chamber_y, chamber_z + 470), (60, 2300, 980), mats["breach_dark"])
    _spawn_cube(actors, "BreachRear", (0, chamber_y - 1150, chamber_z + 470), (2600, 60, 980), mats["breach_dark"])

    crystals = [(-760, -10300, 260, 1.0), (720, -10000, 320, 1.25), (-320, -10700, 210, 0.82), (390, -10820, 180, 0.70)]
    for i, (x, y, h, scale) in enumerate(crystals):
        _spawn_cone(actors, f"BreachCrystal_{i}", (x, y, chamber_z + h * 0.5), 120 * scale, h, mats["breach"], unreal.Rotator(0, i * 27, i * 8))
    _spawn_point_light(actors, "BreachGlowA", (-600, -10300, chamber_z + 350), (105, 54, 255), 1850, 1450, True)
    _spawn_point_light(actors, "BreachGlowB", (650, -10120, chamber_z + 280), (163, 85, 255), 1450, 1200, False)


def _spawn_atmosphere(actors):
    # Remove native procedural director from the bootstrap map for the curated vertical slice.
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label() == "RIFT_AUTO_WorldDirector":
                actors.destroy_actor(actor)
        except Exception:
            pass

    try:
        moon = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2200), unreal.Rotator(-48, -28, 0))
        _label(moon, "Moon")
        comp = _component(moon, unreal.DirectionalLightComponent)
        if comp:
            rw.safe_set(comp, "intensity", 0.08)
            rw.safe_set(comp, "light_color", unreal.Color(88, 111, 168, 255))
            rw.safe_set(comp, "cast_shadows", True)
            rw.safe_set(comp, "volumetric_scattering_intensity", 0.04)
    except Exception:
        pass

    try:
        sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1600), unreal.Rotator())
        _label(sky, "Sky")
        comp = _component(sky, unreal.SkyLightComponent)
        if comp:
            rw.safe_set(comp, "intensity", 0.12)
            rw.safe_set(comp, "light_color", unreal.Color(45, 58, 88, 255))
            rw.safe_set(comp, "real_time_capture", True)
    except Exception:
        pass

    try:
        fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator())
        _label(fog, "Fog")
        comp = _component(fog, unreal.ExponentialHeightFogComponent)
        if comp:
            rw.safe_set(comp, "fog_density", 0.0032)
            rw.safe_set(comp, "fog_height_falloff", 0.16)
            rw.safe_set(comp, "fog_inscattering_luminance", unreal.LinearColor(0.010, 0.016, 0.029, 1.0))
            rw.safe_set(comp, "enable_volumetric_fog", True)
            rw.safe_set(comp, "volumetric_fog_scattering_distribution", 0.25)
            rw.safe_set(comp, "volumetric_fog_extinction_scale", 0.22)
            rw.safe_set(comp, "volumetric_fog_distance", 4200.0)
    except Exception:
        pass


def apply_all() -> None:
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    materials = rv.apply_all()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    # Idempotent art rebuild.
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    _spawn_atmosphere(actors)
    _spawn_surface(actors, materials)
    _spawn_underground(actors, materials)

    level.save_current_level()
    rw.log("Curated visual vertical slice rebuilt: street, interiors, industrial yard and physical underground route")


if __name__ == "__main__":
    apply_all()
