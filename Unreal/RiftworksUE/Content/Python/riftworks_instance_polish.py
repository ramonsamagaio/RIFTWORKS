from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv


def _all_components(actor, cls):
    if not actor:
        return []
    try:
        return list(actor.get_components_by_class(cls))
    except Exception:
        return []


def _set_materials(actor, material, head_material=None):
    if not actor or not material:
        return
    for comp in _all_components(actor, unreal.StaticMeshComponent):
        try:
            name = comp.get_name().lower()
            comp.set_material(0, head_material if head_material and "head" in name else material)
        except Exception:
            pass


def _set_skeletal_material(actor, material):
    if not actor or not material:
        return
    for comp in _all_components(actor, unreal.SkeletalMeshComponent):
        try:
            comp.set_material(0, material)
        except Exception:
            pass


def _quiet_lights(actor):
    if not actor:
        return
    for spot in _all_components(actor, unreal.SpotLightComponent):
        try:
            rw.safe_set(spot, "volumetric_scattering_intensity", 0.06)
            if "work" in spot.get_name().lower():
                rw.safe_set(spot, "intensity", 3400.0)
                rw.safe_set(spot, "attenuation_radius", 2600.0)
        except Exception:
            pass
    for point in _all_components(actor, unreal.PointLightComponent):
        try:
            name = point.get_name().lower()
            rw.safe_set(point, "volumetric_scattering_intensity", 0.05 if "core" not in name else 0.10)
        except Exception:
            pass


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    mats = rv.ensure_material_library()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""

        if label == "RIFT_AUTO_WalkerColossus":
            _set_skeletal_material(actor, mats.get("colossus"))
            _quiet_lights(actor)
        elif label.startswith("RIFT_EXTRA_BreachGolem"):
            _set_materials(actor, mats.get("breach_dark"), mats.get("assembly_motor"))
            _quiet_lights(actor)
        elif label == "RIFT_AUTO_Generator":
            _set_materials(actor, mats.get("rust"))
            _quiet_lights(actor)
        elif label in ("RIFT_AUTO_Battery", "RIFT_AUTO_Floodlight"):
            _set_materials(actor, mats.get("metal"))
            _quiet_lights(actor)
        elif label == "RIFT_AUTO_StarterBase":
            _set_materials(actor, mats.get("metal"))
            _quiet_lights(actor)
        elif label.startswith("RIFT_AUTO_Humanoid"):
            _quiet_lights(actor)

    level.save_current_level()
    rw.log("Instance polish complete: no stale debug materials or legacy volumetric lights")


if __name__ == "__main__":
    apply_all()
