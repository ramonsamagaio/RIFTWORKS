from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_WEATHER_"


def _tag(actor, name):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cube(actors, name, loc, size, mat, yaw=0.0):
    return _tag(vs._spawn_cube(actors, name, loc, size, mat, unreal.Rotator(0, yaw, 0)), name)


def _cone(actors, name, loc, diameter, height, mat, rot=None):
    return _tag(vs._spawn_cone(actors, name, loc, diameter, height, mat, rot or unreal.Rotator()), name)


def _materials():
    puddle = rv.create_surface_material("M_Puddle_BlackWater", (0.004, 0.010, 0.016), 0.06, 0.05, None)
    oil = rv.create_surface_material("M_OilSlick_Dark", (0.016, 0.009, 0.012), 0.10, 0.18, None)
    wet_concrete = rv.create_surface_material("M_Concrete_WetPatch", (0.030, 0.035, 0.034), 0.24, 0.0, None)
    broken_glass = rv.create_surface_material("M_GlassShard_Night", (0.018, 0.060, 0.085), 0.11, 0.08, None)
    ash = rv.create_surface_material("M_Debris_Ash", (0.030, 0.030, 0.028), 0.97, 0.0, None)
    return puddle, oil, wet_concrete, broken_glass, ash


def _road_wetness(actors, puddle, oil):
    puddles = [
        (-210, 3320, 340, 160, -7), (155, 2750, 480, 135, 4), (-245, 1720, 260, 150, 13),
        (110, 840, 390, 180, -9), (-160, -150, 300, 125, 3), (220, -1050, 420, 165, 10),
        (-100, -1900, 520, 150, -4), (175, -3100, 330, 125, 8),
    ]
    for i, (x, y, sx, sy, yaw) in enumerate(puddles):
        _cube(actors, f"Puddle_{i:02d}", (x, y, 12.8), (sx, sy, 2.5), puddle, yaw)
    _cube(actors, "Oil_Workshop", (510, 940, 14.0), (230, 130, 2.2), oil, -12)
    _cube(actors, "Oil_Checkpoint", (-90, -2460, 13.5), (190, 95, 2.2), oil, 18)


def _wall_damp(actors, wet):
    patches = [
        ("WorkshopDampA", (780, 1190, 96), (8, 370, 190), 0),
        ("WorkshopDampB", (2322, 1000, 82), (8, 260, 160), 0),
        ("StoreDamp", (-2168, 1400, 75), (8, 260, 145), 0),
        ("MotelDamp", (-2628, -1500, 90), (8, 410, 175), 0),
        ("UGDampA", (-878, -6900, -650), (8, 560, 220), 0),
        ("UGDampB", (878, -7450, -670), (8, 440, 180), 0),
    ]
    for name, loc, size, yaw in patches:
        _cube(actors, name, loc, size, wet, yaw)


def _broken_glass(actors, glass):
    shards = [
        (-1540, 705, 10, 42, 14, 22), (-1465, 690, 10, 28, 11, -18), (-1380, 715, 10, 50, 10, 7),
        (-1235, 730, 10, 34, 12, 31), (-1820, 720, 10, 32, 8, -25),
    ]
    for i, (x, y, z, length, width, yaw) in enumerate(shards):
        _cube(actors, f"GlassShard_{i:02d}", (x, y, z), (length, width, 3), glass, yaw)


def _debris(actors, mats, ash):
    clusters = [
        ("Checkpoint", (-830, -2070, 0)),
        ("Workshop", (2460, 1480, 0)),
        ("Motel", (-2350, -2050, 0)),
        ("UG", (-650, -7650, -925)),
    ]
    for ci, (name, (x, y, z)) in enumerate(clusters):
        for i in range(8):
            ox = ((i * 73 + ci * 41) % 250) - 125
            oy = ((i * 97 + ci * 29) % 220) - 110
            size = 24 + (i % 4) * 13
            material = ash if i % 3 == 0 else (mats["concrete"] if i % 2 else mats["rust"])
            _cube(actors, f"Debris_{name}_{i:02d}", (x + ox, y + oy, z + size * 0.25),
                  (size * 1.5, size, size * 0.5), material, (i * 37) % 90)


def _breach_fragments(actors, mats):
    fragments = [
        (-1050, -10100, -1320, 95, 220, 16), (1050, -10320, -1310, 70, 170, -11),
        (-960, -10700, -1300, 65, 145, 28), (940, -10800, -1295, 110, 260, -22),
    ]
    for i, (x, y, z, d, h, pitch) in enumerate(fragments):
        _cone(actors, f"BreachFragment_{i}", (x, y, z + h * 0.5), d, h, mats["breach"], unreal.Rotator(pitch, i * 41, 0))


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    mats = rv.ensure_material_library()
    puddle, oil, wet, glass, ash = _materials()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    _road_wetness(actors, puddle, oil)
    _wall_damp(actors, wet)
    _broken_glass(actors, glass)
    _debris(actors, mats, ash)
    _breach_fragments(actors, mats)
    level.save_current_level()
    rw.log("Weathering pass complete: wetness, glass, oil, debris and Breach fragments")


if __name__ == "__main__":
    apply_all()
