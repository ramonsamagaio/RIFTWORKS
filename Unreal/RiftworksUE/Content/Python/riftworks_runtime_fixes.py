from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv


def _pick(animations, candidates):
    normalized = [(asset, rw.normalize(asset.get_name())) for asset in animations]
    for candidate in candidates:
        key = rw.normalize(candidate)
        for asset, name in normalized:
            if name == key or key in name or name in key:
                return asset
    return None


def _copy(source, target, prop):
    if not source or not target:
        return
    try:
        target.set_editor_property(prop, source.get_editor_property(prop))
    except Exception:
        pass


def _component_mesh(component):
    if not component:
        return None
    for prop in ("skeletal_mesh_asset", "skeletal_mesh"):
        try:
            mesh = component.get_editor_property(prop)
            if mesh:
                return mesh
        except Exception:
            pass
    return None


def _copy_player_defaults(source, target):
    for prop in [
        "health", "walk_speed", "sprint_speed", "field_of_view",
        "flashlight_battery", "flashlight_drain_per_second", "b_flashlight_on",
        "rifle_damage", "rifle_range", "scrap", "components",
        "idle_animation", "walk_animation", "run_animation", "crouch_animation", "pistol_shoot_animation",
        "max_stamina", "stamina", "sprint_drain_per_second", "stamina_recovery_per_second",
        "sprint_recovery_threshold", "sprint_fov_boost", "camera_interp_speed",
        "walk_bob_amplitude", "sprint_bob_amplitude", "bob_frequency",
        "engineering_trace_distance", "selected_joint_mode", "selected_utility_build_mode",
        "logic_button_class", "logic_sensor_class", "logic_timer_class",
    ]:
        _copy(source, target, prop)

    try:
        source_mesh = source.get_editor_property("mesh")
        target_mesh = target.get_editor_property("mesh")
        mesh_asset = _component_mesh(source_mesh)
        if mesh_asset:
            rw.set_skeletal_mesh(target_mesh, mesh_asset)
    except Exception:
        pass


def _configure_humanoid(mesh, skeleton, animations):
    path = f"{rw.BP_DIR}/BP_RiftAnimatedHumanoid"
    rw.create_blueprint("BP_RiftAnimatedHumanoid", rw.BP_DIR, "/Script/RiftworksUE.RiftAnimatedHumanoidNPC")
    cdo = rw.blueprint_cdo(path)
    if not cdo or not mesh:
        return path

    try:
        rw.set_skeletal_mesh(cdo.get_editor_property("mesh"), mesh, 92.0)
    except TypeError:
        rw.set_skeletal_mesh(cdo.get_editor_property("mesh"), mesh)
    except Exception:
        pass

    clips = {
        "idle_animation": _pick(animations, ["Idle_Loop", "Idle"]),
        "walk_animation": _pick(animations, ["Walking", "Walk_Loop", "Walk"]),
        "run_animation": _pick(animations, ["Fast Run", "Sprint_Loop", "Jog_Fwd_Loop", "Jog"]),
        "pistol_idle_animation": _pick(animations, ["Pistol_Idle_Loop", "Pistol Idle", "Idle_Loop"]),
        "pistol_shoot_animation": _pick(animations, ["Pistol_Shoot", "Pistol Shoot"]),
        "hit_animation": _pick(animations, ["Hit_Chest", "Hit_Head", "Hit"]),
        "death_animation": _pick(animations, ["Death01", "Death"]),
    }
    for prop, clip in clips.items():
        if clip:
            rw.safe_set(cdo, prop, clip)

    rw.safe_set(cdo, "b_use_single_node_animation_fallback", True)
    rw.safe_set(cdo, "walk_speed", 185.0)
    rw.safe_set(cdo, "combat_speed", 330.0)

    mats = rv.ensure_material_library()
    try:
        mesh_component = cdo.get_editor_property("mesh")
        if mats.get("hazard"):
            mesh_component.set_material(0, mats["hazard"])
    except Exception:
        pass

    rw.asset_library.save_asset(path, only_if_is_dirty=False)
    return path


def _replace_humanoids(path):
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    cls = rw.blueprint_class(path)
    if not cls:
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    replacements = []
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        if not label.startswith("RIFT_AUTO_Humanoid_"):
            continue
        replacements.append((label, actor.get_actor_location(), actor.get_actor_rotation()))
        actors.destroy_actor(actor)

    for label, location, rotation in replacements:
        npc = actors.spawn_actor_from_class(cls, location, rotation)
        if npc:
            npc.set_actor_label(label)

    level.save_current_level()
    rw.log(f"Replaced {len(replacements)} humanoids with Colossus-style grounded animation runtime")


def _configure_survival_player():
    path = f"{rw.BP_DIR}/BP_RiftSurvivalPlayer"
    rw.create_blueprint("BP_RiftSurvivalPlayer", rw.BP_DIR, "/Script/RiftworksUE.RiftSurvivalPlayerCharacter")
    target = rw.blueprint_cdo(path)
    source = rw.blueprint_cdo(f"{rw.BP_DIR}/BP_RiftEngineeringPlayer")
    if target and source:
        _copy_player_defaults(source, target)
        try:
            battery = float(target.get_editor_property("flashlight_battery"))
            rw.safe_set(target, "flashlight_battery", max(25.0, battery))
        except Exception:
            rw.safe_set(target, "flashlight_battery", 100.0)
        rw.safe_set(target, "b_flashlight_on", True)
        try:
            light = target.get_editor_property("flashlight")
            rw.safe_set(light, "light_function_material", None)
            rw.safe_set(light, "intensity", 1150.0)
            rw.safe_set(light, "attenuation_radius", 5000.0)
            rw.safe_set(light, "inner_cone_angle", 14.0)
            rw.safe_set(light, "outer_cone_angle", 32.0)
            rw.safe_set(light, "source_radius", 1.2)
            rw.safe_set(light, "soft_source_radius", 5.0)
            rw.safe_set(light, "temperature", 4500.0)
            rw.safe_set(light, "volumetric_scattering_intensity", 0.008)
            rw.safe_set(light, "cast_shadows", True)
        except Exception:
            pass
        rw.asset_library.save_asset(path, only_if_is_dirty=False)

    game_mode = rw.blueprint_cdo(f"{rw.BP_DIR}/BP_RiftGameMode")
    cls = rw.blueprint_class(path)
    if game_mode and cls:
        rw.safe_set(game_mode, "default_pawn_class", cls)
        rw.asset_library.save_asset(f"{rw.BP_DIR}/BP_RiftGameMode", only_if_is_dirty=False)
    return path


def apply_all():
    mesh, skeleton, animations, score = rw.runtime_character_assets()
    if mesh and skeleton and animations:
        humanoid_path = _configure_humanoid(mesh, skeleton, animations)
        _replace_humanoids(humanoid_path)
        rw.log(f"Humanoid runtime locked to same Skeleton-native asset family as Colossus: {mesh.get_name()} ({score} clips)")
    else:
        rw.warn("Runtime humanoid fix waiting for a Skeleton-native mesh/animation pair")

    _configure_survival_player()
    rw.log("Stable flashlight + visual inventory player layer configured")


if __name__ == "__main__":
    apply_all()
