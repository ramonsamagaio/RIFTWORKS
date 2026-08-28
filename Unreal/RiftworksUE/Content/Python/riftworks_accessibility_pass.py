from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_ACCESS_"


def _label(actor, name):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cube(actors, name, loc, size, mat, rot=None):
    actor = vs._spawn_cube(actors, name, loc, size, mat, rot)
    return _label(actor, name)


def _point(actors, name, loc, color, intensity, radius):
    actor = vs._spawn_point_light(actors, name, loc, color, intensity, radius, False)
    return _label(actor, name)


def _remove_old_access_geometry(actors):
    prefixes = (
        "RIFT_ART_UG_Step_",
        "RIFT_ART_UG_WallL_",
        "RIFT_ART_UG_WallR_",
        "RIFT_ART_UG_EmergencyLamp_",
        "RIFT_ART_UG_EmergencyLight_",
        "RIFT_ART_DeepStep_",
        "RIFT_ART_DeepRailL_",
        "RIFT_ART_DeepRailR_",
    )
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        if label.startswith(PREFIX) or any(label.startswith(prefix) for prefix in prefixes):
            actors.destroy_actor(actor)


def _build_surface_to_station(actors, mats):
    # Landing bridges the road edge to the stair portal. 35 cm rise/run increments stay under Character step height.
    _cube(actors, "EntryLanding", (0, -4120, -2), (760, 360, 22), mats["concrete"])

    step_count = 26
    y0 = -4300.0
    z0 = -12.0
    run = 82.0
    drop = 35.0
    for i in range(step_count):
        y = y0 - i * run
        z = z0 - i * drop
        _cube(actors, f"StationStep_{i:02d}", (0, y, z), (760, 94, 22), mats["concrete"])
        _cube(actors, f"StationWallL_{i:02d}", (-410, y, z + 185), (34, 100, 390), mats["concrete_dark"])
        _cube(actors, f"StationWallR_{i:02d}", (410, y, z + 185), (34, 100, 390), mats["concrete_dark"])
        if i % 5 == 1:
            _cube(actors, f"StationLamp_{i:02d}", (-382, y, z + 278), (20, 20, 30), mats["emergency"])
            _point(actors, f"StationLight_{i:02d}", (-340, y, z + 250), (255, 44, 26), 150, 360)

    end_y = y0 - (step_count - 1) * run
    end_z = z0 - (step_count - 1) * drop
    # Transition corridor into the maintenance hall at ~Z -925.
    corridor_y = (end_y + -6900.0) * 0.5
    corridor_len = abs(-6900.0 - end_y) + 160.0
    _cube(actors, "StationTransitionFloor", (0, corridor_y, -913), (760, corridor_len, 24), mats["concrete_dark"])
    _cube(actors, "StationTransitionL", (-410, corridor_y, -675), (34, corridor_len, 500), mats["concrete"])
    _cube(actors, "StationTransitionR", (410, corridor_y, -675), (34, corridor_len, 500), mats["concrete"])
    _cube(actors, "StationTransitionRoof", (0, corridor_y, -428), (850, corridor_len, 30), mats["metal"])


def _build_station_to_breach(actors, mats):
    # Clear connector from station hall into the second descent.
    _cube(actors, "DeepLanding", (0, -8180, -918), (820, 620, 24), mats["metal"])

    step_count = 12
    y0 = -8420.0
    z0 = -940.0
    run = 78.0
    drop = 34.0
    for i in range(step_count):
        y = y0 - i * run
        z = z0 - i * drop
        _cube(actors, f"DeepStepSafe_{i:02d}", (0, y, z), (730, 90, 22), mats["metal"])
        for side in (-1, 1):
            _cube(actors, f"DeepRailPost_{i:02d}_{side}", (side * 350, y, z + 82), (14, 18, 165), mats["rust"])
        if i in (2, 7, 11):
            _cube(actors, f"DeepMarker_{i:02d}", (330, y, z + 115), (16, 16, 32), mats["breach"])

    end_y = y0 - (step_count - 1) * run
    end_z = z0 - (step_count - 1) * drop
    chamber_y = -9900.0
    corridor_y = (end_y + chamber_y) * 0.5
    corridor_len = abs(chamber_y - end_y) + 240.0
    _cube(actors, "DeepToBreachFloor", (0, corridor_y, -1332), (780, corridor_len, 24), mats["breach_dark"])
    for side in (-1, 1):
        _cube(actors, f"DeepToBreachWall_{side}", (side * 405, corridor_y, -1090), (30, corridor_len, 500), mats["breach_dark"])
    _point(actors, "DeepTransitionGlow", (0, -9450, -1110), (95, 56, 220), 360, 700)


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    mats = rv.ensure_material_library()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)
    _remove_old_access_geometry(actors)
    _build_surface_to_station(actors, mats)
    _build_station_to_breach(actors, mats)
    level.save_current_level()
    rw.log("Accessibility pass complete: walkable surface -> station -> Breach route")


if __name__ == "__main__":
    apply_all()
