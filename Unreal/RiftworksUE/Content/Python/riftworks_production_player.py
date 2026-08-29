from __future__ import annotations

import unreal
import riftworks_setup as rw


def _copy_property(source, target, prop):
    try:
        value = source.get_editor_property(prop)
        target.set_editor_property(prop, value)
    except Exception:
        pass


def _copy_visual_components(source, target):
    try:
        source_mesh = source.get_editor_property("mesh")
        target_mesh = target.get_editor_property("mesh")
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
        target_light = target.get_editor_property("flashlight")
        if source_light and target_light:
            for light_prop in [
                "light_function_material",
                "intensity",
                "attenuation_radius",
                "inner_cone_angle",
                "outer_cone_angle",
                "volumetric_scattering_intensity",
                "temperature",
            ]:
                try:
                    target_light.set_editor_property(light_prop, source_light.get_editor_property(light_prop))
                except Exception:
                    pass
    except Exception:
        pass


def apply_all():
    source_path = f"{rw.BP_DIR}/BP_RiftPlayer"
    production_path = f"{rw.BP_DIR}/BP_RiftProductionPlayer"
    engineering_path = f"{rw.BP_DIR}/BP_RiftEngineeringPlayer"
    game_mode_path = f"{rw.BP_DIR}/BP_RiftGameMode"

    rw.create_blueprint("BP_RiftProductionPlayer", rw.BP_DIR, "/Script/RiftworksUE.RiftProductionPlayerCharacter")
    rw.create_blueprint("BP_RiftEngineeringPlayer", rw.BP_DIR, "/Script/RiftworksUE.RiftEngineeringPlayerCharacter")

    source = rw.blueprint_cdo(source_path)
    production = rw.blueprint_cdo(production_path)
    engineering = rw.blueprint_cdo(engineering_path)
    if not source or not production or not engineering:
        rw.log("Production/engineering player pass waiting for BP_RiftPlayer/native classes")
        return

    shared_props = [
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
    ]
    for prop in shared_props:
        _copy_property(source, production, prop)

    _copy_visual_components(source, production)

    rw.safe_set(production, "max_stamina", 100.0)
    rw.safe_set(production, "stamina", 100.0)
    rw.safe_set(production, "sprint_drain_per_second", 18.0)
    rw.safe_set(production, "stamina_recovery_per_second", 14.0)
    rw.safe_set(production, "walk_bob_amplitude", 0.85)
    rw.safe_set(production, "sprint_bob_amplitude", 1.25)
    rw.safe_set(production, "engineering_trace_distance", 1350.0)
    rw.asset_library.save_asset(production_path, only_if_is_dirty=False)

    # Final player inherits the native engineering/input layer but receives the same mannequin,
    # animation and flashlight defaults as the production controller Blueprint.
    for prop in shared_props + [
        "max_stamina",
        "stamina",
        "sprint_drain_per_second",
        "stamina_recovery_per_second",
        "sprint_recovery_threshold",
        "sprint_fov_boost",
        "camera_interp_speed",
        "walk_bob_amplitude",
        "sprint_bob_amplitude",
        "bob_frequency",
        "engineering_trace_distance",
    ]:
        _copy_property(production, engineering, prop)
    _copy_visual_components(production, engineering)

    logic_button_cls = rw.blueprint_class(f"{rw.GAMEPLAY_BP_DIR}/BP_RiftLogicButton")
    logic_sensor_cls = rw.blueprint_class(f"{rw.GAMEPLAY_BP_DIR}/BP_RiftLogicSensor")
    logic_timer_cls = rw.blueprint_class(f"{rw.GAMEPLAY_BP_DIR}/BP_RiftLogicTimer")
    if logic_button_cls:
        rw.safe_set(engineering, "logic_button_class", logic_button_cls)
    if logic_sensor_cls:
        rw.safe_set(engineering, "logic_sensor_class", logic_sensor_cls)
    if logic_timer_cls:
        rw.safe_set(engineering, "logic_timer_class", logic_timer_cls)

    rw.asset_library.save_asset(engineering_path, only_if_is_dirty=False)

    game_mode = rw.blueprint_cdo(game_mode_path)
    engineering_cls = rw.blueprint_class(engineering_path)
    if game_mode and engineering_cls:
        rw.safe_set(game_mode, "default_pawn_class", engineering_cls)
        rw.asset_library.save_asset(game_mode_path, only_if_is_dirty=False)

    rw.log("Final FPS player active: stamina + material build costs + generic joints + player-built logic utilities and signal links")


if __name__ == "__main__":
    apply_all()
