from __future__ import annotations

import unreal
import riftworks_setup as rw


def _copy_property(source, target, prop):
    try:
        value = source.get_editor_property(prop)
        target.set_editor_property(prop, value)
    except Exception:
        pass


def apply_all():
    source_path = f"{rw.BP_DIR}/BP_RiftPlayer"
    production_path = f"{rw.BP_DIR}/BP_RiftProductionPlayer"
    game_mode_path = f"{rw.BP_DIR}/BP_RiftGameMode"

    rw.create_blueprint("BP_RiftProductionPlayer", rw.BP_DIR, "/Script/RiftworksUE.RiftProductionPlayerCharacter")

    source = rw.blueprint_cdo(source_path)
    production = rw.blueprint_cdo(production_path)
    if not source or not production:
        rw.log("Production player pass waiting for BP_RiftPlayer/native class")
        return

    for prop in [
        "health",
        "walk_speed",
        "sprint_speed",
        "field_of_view",
        "flashlight_battery",
        "flashlight_drain_per_second",
        "b_flashlight_on",
        "rifle_damage",
        "rifle_range",
        "scrap",
        "idle_animation",
        "walk_animation",
        "run_animation",
        "crouch_animation",
        "pistol_shoot_animation",
    ]:
        _copy_property(source, production, prop)

    try:
        source_mesh = source.get_editor_property("mesh")
        target_mesh = production.get_editor_property("mesh")
        if source_mesh and target_mesh:
            for mesh_prop in ["skeletal_mesh_asset", "skeletal_mesh"]:
                try:
                    target_mesh.set_editor_property(mesh_prop, source_mesh.get_editor_property(mesh_prop))
                    break
                except Exception:
                    pass
    except Exception:
        pass

    try:
        source_light = source.get_editor_property("flashlight")
        target_light = production.get_editor_property("flashlight")
        if source_light and target_light:
            for light_prop in [
                "light_function_material",
                "intensity",
                "attenuation_radius",
                "inner_cone_angle",
                "outer_cone_angle",
                "volumetric_scattering_intensity",
            ]:
                try:
                    target_light.set_editor_property(light_prop, source_light.get_editor_property(light_prop))
                except Exception:
                    pass
    except Exception:
        pass

    rw.safe_set(production, "max_stamina", 100.0)
    rw.safe_set(production, "stamina", 100.0)
    rw.safe_set(production, "sprint_drain_per_second", 18.0)
    rw.safe_set(production, "stamina_recovery_per_second", 14.0)
    rw.safe_set(production, "walk_bob_amplitude", 0.85)
    rw.safe_set(production, "sprint_bob_amplitude", 1.25)
    rw.asset_library.save_asset(production_path, only_if_is_dirty=False)

    game_mode = rw.blueprint_cdo(game_mode_path)
    production_cls = rw.blueprint_class(production_path)
    if game_mode and production_cls:
        rw.safe_set(game_mode, "default_pawn_class", production_cls)
        rw.asset_library.save_asset(game_mode_path, only_if_is_dirty=False)

    rw.log("Production FPS controller active: stamina, sprint recovery, smooth crouch and restrained camera motion")


if __name__ == "__main__":
    apply_all()
