from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_LANDMARK_"


def _tag(actor, name):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cube(actors, name, loc, size, mat, rot=None):
    return _tag(vs._spawn_cube(actors, name, loc, size, mat, rot or rw.rotator()), name)


def _cyl(actors, name, loc, d, h, mat, rot=None):
    return _tag(vs._spawn_cylinder(actors, name, loc, d, h, mat, rot or rw.rotator()), name)


def _cone(actors, name, loc, d, h, mat, rot=None):
    return _tag(vs._spawn_cone(actors, name, loc, d, h, mat, rot or rw.rotator()), name)


def _light(actors, name, loc, color, intensity, radius):
    return _tag(vs._spawn_point_light(actors, name, loc, color, intensity, radius, False), name)


def _radio_tower(actors, mats):
    x, y, z = 3300, 3100, 0
    height = 2400
    levels = 9
    for level in range(levels):
        t = level / float(levels - 1)
        zz = z + 100 + t * height
        half = 260 * (1.0 - 0.62 * t)
        for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            _cube(actors, f"RadioLeg_{level}_{sx}_{sy}", (x + sx * half, y + sy * half, zz), (28, 28, 330), mats["metal"])
        if level < levels - 1:
            for side in range(4):
                if side % 2 == 0:
                    _cube(actors, f"RadioBraceX_{level}_{side}", (x, y + (-half if side == 0 else half), zz + 135), (half * 2, 22, 22), mats["rust"])
                else:
                    _cube(actors, f"RadioBraceY_{level}_{side}", (x + (-half if side == 1 else half), y, zz + 135), (22, half * 2, 22), mats["rust"])
        if level in (3, 6):
            _cube(actors, f"RadioPlatform_{level}", (x, y, zz + 120), (half * 2.35, half * 2.35, 20), mats["metal"])
    _cyl(actors, "RadioAntenna", (x, y, z + height + 350), 34, 700, mats["metal"])
    for i, zz in enumerate((900, 1650, 2460)):
        _cube(actors, f"RadioRedBeacon_{i}", (x, y, z + zz), (34, 34, 34), mats["emergency"])
        _light(actors, f"RadioRedLight_{i}", (x, y, z + zz), (255, 22, 18), 125, 420)


def _water_tower(actors, mats):
    x, y, z = -3400, 2350, 0
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        _cube(actors, f"WaterLeg_{sx}_{sy}", (x + sx * 235, y + sy * 235, z + 620), (38, 38, 1240), mats["rust"])
    for level in (320, 760, 1120):
        _cube(actors, f"WaterBraceX_{level}", (x, y - 235, z + level), (470, 24, 24), mats["metal"])
        _cube(actors, f"WaterBraceY_{level}", (x - 235, y, z + level), (24, 470, 24), mats["metal"])
    _cyl(actors, "WaterTank", (x, y, z + 1450), 850, 560, mats["metal"])
    _cone(actors, "WaterTankRoof", (x, y, z + 1785), 900, 170, mats["rust"])
    _cube(actors, "WaterCatwalk", (x, y, z + 1240), (1040, 1040, 22), mats["metal"])
    _cube(actors, "WaterServiceLamp", (x + 420, y, z + 1290), (28, 32, 28), mats["lamp"])
    _light(actors, "WaterServiceLight", (x + 390, y, z + 1260), (255, 185, 92), 180, 520)


def _industrial_stack(actors, mats):
    x, y, z = 3600, -1700, 0
    _cyl(actors, "StackBase", (x, y, z + 450), 480, 900, mats["concrete_dark"])
    _cyl(actors, "StackMid", (x, y, z + 1200), 350, 650, mats["rust"])
    _cyl(actors, "StackTop", (x, y, z + 1780), 250, 520, mats["metal"])
    for h in (750, 1350, 1950):
        _cube(actors, f"StackBand_{h}", (x, y, z + h), (540 if h == 750 else 390, 540 if h == 750 else 390, 30), mats["hazard"])
    _cube(actors, "StackBeacon", (x, y, z + 2055), (35, 35, 35), mats["emergency"])
    _light(actors, "StackBeaconLight", (x, y, z + 2050), (255, 26, 18), 135, 420)


def _breach_spire(actors, mats):
    x, y, z = -4300, -4700, 0
    _cube(
        actors,
        "SpirePlinth",
        (x, y, z + 55),
        (850, 850, 110),
        mats["breach_dark"],
        rw.rotator(yaw=17.0),
    )
    for i, (offset, height, width, pitch) in enumerate((
        ((0, 0), 1700, 290, -5), ((240, 80), 1050, 190, 11), ((-250, 130), 920, 165, -14),
        ((90, -260), 680, 135, 8),
    )):
        ox, oy = offset
        _cone(
            actors,
            f"SpireShard_{i}",
            (x + ox, y + oy, z + height * 0.5),
            width,
            height,
            mats["breach"] if i == 0 else mats["breach_dark"],
            rw.rotator(pitch=float(pitch), yaw=float(i * 37)),
        )
    _light(actors, "SpireGlow", (x, y, z + 880), (96, 55, 255), 520, 1450)


def _subway_beacon(actors, mats):
    x, y, z = 0, -4160, 0
    _cube(actors, "SubwaySignPostL", (-260, y, z + 210), (34, 34, 420), mats["metal"])
    _cube(actors, "SubwaySignPostR", (260, y, z + 210), (34, 34, 420), mats["metal"])
    _cube(actors, "SubwaySignBody", (0, y, z + 400), (610, 42, 130), mats["rust"])
    _cube(actors, "SubwaySignGlow", (0, y - 24, z + 400), (420, 8, 62), mats["emergency"])
    _light(actors, "SubwaySignLight", (0, y - 70, z + 385), (222, 42, 28), 200, 520)


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    mats = rv.ensure_material_library()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass
    _radio_tower(actors, mats)
    _water_tower(actors, mats)
    _industrial_stack(actors, mats)
    _breach_spire(actors, mats)
    _subway_beacon(actors, mats)
    level.save_current_level()
    rw.log("Landmark pass complete: radio mast, water tower, stack, Breach spire and subway beacon")


if __name__ == "__main__":
    apply_all()
