from __future__ import annotations

import unreal
import riftworks_setup as rw


def _components(actor, cls):
    if not actor:
        return []
    try:
        return list(actor.get_components_by_class(cls))
    except Exception:
        return []


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    fog_count = 0
    light_count = 0
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""

        if label == "RIFT_ART_Fog":
            for comp in _components(actor, unreal.ExponentialHeightFogComponent):
                rw.safe_set(comp, "fog_density", 0.0014)
                rw.safe_set(comp, "fog_height_falloff", 0.22)
                rw.safe_set(comp, "fog_inscattering_luminance", unreal.LinearColor(0.006, 0.010, 0.018, 1.0))
                rw.safe_set(comp, "enable_volumetric_fog", True)
                rw.safe_set(comp, "volumetric_fog_scattering_distribution", 0.18)
                rw.safe_set(comp, "volumetric_fog_extinction_scale", 0.10)
                rw.safe_set(comp, "volumetric_fog_distance", 3000.0)
                fog_count += 1

        elif label == "RIFT_ART_Moon":
            for comp in _components(actor, unreal.DirectionalLightComponent):
                rw.safe_set(comp, "intensity", 0.10)
                rw.safe_set(comp, "light_color", unreal.Color(78, 101, 155, 255))
                rw.safe_set(comp, "volumetric_scattering_intensity", 0.015)
                rw.safe_set(comp, "cast_shadows", True)
                light_count += 1

        elif label == "RIFT_ART_Sky":
            for comp in _components(actor, unreal.SkyLightComponent):
                rw.safe_set(comp, "intensity", 0.10)
                rw.safe_set(comp, "light_color", unreal.Color(39, 50, 78, 255))
                light_count += 1

        # Keep all generated practical lights readable but never smoky.
        if label.startswith(("RIFT_ART_", "RIFT_DRESS_", "RIFT_BEAUTY_", "RIFT_LIVED_", "RIFT_LANDMARK_")):
            for comp in _components(actor, unreal.PointLightComponent):
                rw.safe_set(comp, "volumetric_scattering_intensity", 0.025)
            for comp in _components(actor, unreal.SpotLightComponent):
                rw.safe_set(comp, "volumetric_scattering_intensity", 0.035)

    level.save_current_level()
    rw.log(f"Atmosphere refine complete: subtle depth fog ({fog_count}) and controlled night lighting ({light_count})")


if __name__ == "__main__":
    apply_all()
