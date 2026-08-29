from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_GDD_NETWORK_"


def _material(component, material):
    if component and material:
        try:
            component.set_material(0, material)
        except Exception:
            pass


def _spawn(actors, cls, label, location, rotation=(0.0, 0.0, 0.0)):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor:
        try:
            actor.set_actor_label(PREFIX + label)
        except Exception:
            pass
    return actor


def _ensure_blueprints(materials):
    outpost_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftOutpost"
    thermal_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftThermalField"
    cryo_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftCryoField"
    phase_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftPhaseField"

    rw.create_blueprint("BP_RiftOutpost", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftOutpostBeacon")
    rw.create_blueprint("BP_RiftThermalField", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftTemperatureField")
    rw.create_blueprint("BP_RiftCryoField", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftTemperatureField")
    rw.create_blueprint("BP_RiftPhaseField", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftPhaseField")

    outpost = rw.blueprint_cdo(outpost_path)
    if outpost:
        try:
            _material(outpost.get_editor_property("mesh"), materials.get("metal"))
        except Exception:
            pass
        rw.safe_set(outpost, "storage_capacity_units", 36)
        rw.asset_library.save_asset(outpost_path, only_if_is_dirty=False)

    enum_type = getattr(unreal, "RiftTemperatureFieldMode", None)
    thermal = rw.blueprint_cdo(thermal_path)
    if thermal:
        if enum_type and hasattr(enum_type, "THERMAL"):
            rw.safe_set(thermal, "mode", enum_type.THERMAL)
        try:
            _material(thermal.get_editor_property("core_mesh"), materials.get("hazard"))
        except Exception:
            pass
        rw.safe_set(thermal, "radius", 760.0)
        rw.asset_library.save_asset(thermal_path, only_if_is_dirty=False)

    cryo = rw.blueprint_cdo(cryo_path)
    if cryo:
        if enum_type and hasattr(enum_type, "CRYO"):
            rw.safe_set(cryo, "mode", enum_type.CRYO)
        try:
            _material(cryo.get_editor_property("core_mesh"), materials.get("breach_dark"))
        except Exception:
            pass
        rw.safe_set(cryo, "radius", 820.0)
        rw.safe_set(cryo, "cryo_character_speed", 145.0)
        rw.asset_library.save_asset(cryo_path, only_if_is_dirty=False)

    phase = rw.blueprint_cdo(phase_path)
    if phase:
        try:
            _material(phase.get_editor_property("core_mesh"), materials.get("breach"))
        except Exception:
            pass
        rw.safe_set(phase, "radius", 620.0)
        rw.asset_library.save_asset(phase_path, only_if_is_dirty=False)

    return outpost_path, thermal_path, cryo_path, phase_path


def _spawn_fragile_barricade(actors, materials, label, location, rotation=(0.0, 0.0, 0.0)):
    try:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
        if not actor:
            return None
        actor.set_actor_label(PREFIX + label)
        try:
            actor.set_editor_property("tags", [unreal.Name("RiftFragile")])
        except Exception:
            pass
        component = actor.static_mesh_component
        component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
        component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        actor.set_actor_scale3d(unreal.Vector(1.8, 0.22, 0.72))
        _material(component, materials.get("hazard"))
        return actor
    except Exception as exc:
        rw.warn(f"Fragile barricade spawn failed: {exc}")
        return None


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    materials = rv.ensure_material_library()
    outpost_path, thermal_path, cryo_path, phase_path = _ensure_blueprints(materials)
    outpost_cls = rw.blueprint_class(outpost_path)
    thermal_cls = rw.blueprint_class(thermal_path)
    cryo_cls = rw.blueprint_class(cryo_path)
    phase_cls = rw.blueprint_class(phase_path)
    if not outpost_cls or not thermal_cls or not cryo_cls or not phase_cls:
        rw.log("Network/temperature/phase pass waiting for native classes")
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    station = _spawn(actors, outpost_cls, "StationOutpost", (-620, -7150, -770), (0.0, 20.0, 0.0))
    if station:
        rw.safe_set(station, "outpost_name", "Metro Recovery Outpost")
        rw.safe_set(station, "storage_capacity_units", 42)
        rw.safe_set(station, "access_radius", 1050.0)

    hunt = _spawn(actors, outpost_cls, "HuntingOutpost", (3080, -3220, 72), (0.0, -40.0, 0.0))
    if hunt:
        rw.safe_set(hunt, "outpost_name", "Walker Observation Post")
        rw.safe_set(hunt, "storage_capacity_units", 24)
        rw.safe_set(hunt, "access_radius", 820.0)

    thermal = _spawn(actors, thermal_cls, "ThermalField", (1580, -10350, -1160))
    if thermal:
        rw.safe_set(thermal, "radius", 760.0)
        rw.safe_set(thermal, "thermal_damage_per_second", 8.0)
        rw.safe_set(thermal, "thermal_lift_force", 135000.0)

    cryo = _spawn(actors, cryo_cls, "CryoField", (-1420, -10450, -1160))
    if cryo:
        rw.safe_set(cryo, "radius", 840.0)
        rw.safe_set(cryo, "cryo_drag_force", 112000.0)
        rw.safe_set(cryo, "cryo_character_speed", 145.0)

    phase = _spawn(actors, phase_cls, "PhaseField", (120, -11150, -1240))
    if phase:
        rw.safe_set(phase, "radius", 680.0)
        rw.safe_set(phase, "b_enabled", True)

    for index, offset in enumerate((-600, -300, 0, 300, 600)):
        _spawn_fragile_barricade(
            actors,
            materials,
            f"WalkerBarricade{index:02d}",
            (5000 + offset, -4700 + index * 95, 90),
            (0.0, 15.0 + index * 8.0, 0.0),
        )

    level.save_current_level()
    try:
        rw.asset_library.save_directory(rw.ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    rw.log("GDD network pass ready: outposts, Thermal/Cryo/Phase physics fields and Walker trampling props")


if __name__ == "__main__":
    apply_all()
