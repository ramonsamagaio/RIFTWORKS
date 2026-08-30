from __future__ import annotations

import unreal
import riftworks_setup as rw


def log(msg: str) -> None:
    unreal.log(f"[RIFTWORKS POLISH] {msg}")


def _pick(animations, names):
    normalized = [(asset, rw.normalize(asset.get_name())) for asset in animations]
    for candidate in names:
        key = rw.normalize(candidate)
        for asset, name in normalized:
            if name == key or key in name or name in key:
                return asset
    return None


def refresh_character_animation_defaults() -> None:
    player_path = f"{rw.BP_DIR}/BP_RiftPlayer"
    npc_path = f"{rw.BP_DIR}/BP_RiftHumanoid"
    mesh, skeleton, animations, score = rw.runtime_character_assets()
    if not mesh or not skeleton or not animations:
        log("Character polish skipped: no Skeleton-native mesh/animation pair is available")
        npc = rw.blueprint_cdo(npc_path)
        if npc:
            rw.safe_set(npc, "b_use_single_node_animation_fallback", False)
            rw.asset_library.save_asset(npc_path, only_if_is_dirty=False)
        return

    idle = _pick(animations, ["Idle_Loop", "Idle"])
    walk = _pick(animations, ["Walking", "Walk_Loop", "Walk"])
    run = _pick(animations, ["Fast Run", "Sprint_Loop", "Jog_Fwd_Loop", "Jog"])
    crouch = _pick(animations, ["Crouch_Fwd_Loop", "Crouch_Idle_Loop"])
    pistol_idle = _pick(animations, ["Pistol_Idle_Loop", "Pistol Idle", "Idle_Loop"])
    pistol_shoot = _pick(animations, ["Pistol_Shoot", "Pistol Shoot"])
    hit = _pick(animations, ["Hit_Chest", "Hit Chest", "Hit_Head", "Hit"])
    death = _pick(animations, ["Death01", "Death"])

    player = rw.blueprint_cdo(player_path)
    if player:
        try:
            rw.set_skeletal_mesh(player.get_editor_property("mesh"), mesh)
        except Exception:
            pass
        for prop, value in (("idle_animation", idle), ("walk_animation", walk), ("run_animation", run),
                            ("crouch_animation", crouch), ("pistol_shoot_animation", pistol_shoot)):
            if value:
                rw.safe_set(player, prop, value)
        try:
            flashlight = player.get_editor_property("flashlight")
            rw.safe_set(flashlight, "light_function_material", None)
            rw.safe_set(flashlight, "intensity", 1550.0)
            rw.safe_set(flashlight, "attenuation_radius", 5200.0)
            rw.safe_set(flashlight, "inner_cone_angle", 13.0)
            rw.safe_set(flashlight, "outer_cone_angle", 24.0)
            rw.safe_set(flashlight, "volumetric_scattering_intensity", 0.12)
        except Exception:
            pass
        rw.asset_library.save_asset(player_path, only_if_is_dirty=False)

    npc = rw.blueprint_cdo(npc_path)
    if npc:
        try:
            rw.set_skeletal_mesh(npc.get_editor_property("mesh"), mesh)
        except Exception:
            pass
        for prop, value in (("idle_animation", idle), ("walk_animation", walk), ("run_animation", run),
                            ("pistol_idle_animation", pistol_idle), ("pistol_shoot_animation", pistol_shoot),
                            ("hit_animation", hit), ("death_animation", death)):
            if value:
                rw.safe_set(npc, prop, value)
        rw.safe_set(npc, "b_use_single_node_animation_fallback", True)
        rw.safe_set(npc, "health", 50.0)
        try:
            muzzle = npc.get_editor_property("weapon_muzzle_light")
            rw.safe_set(muzzle, "volumetric_scattering_intensity", 0.10)
        except Exception:
            pass
        rw.asset_library.save_asset(npc_path, only_if_is_dirty=False)

    log(f"Skeleton-safe animation refresh: mesh={mesh.get_name()} native_clips={score}")


def reduce_legacy_volumetrics() -> None:
    paths = [
        f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBaseBeacon",
        f"{rw.GAMEPLAY_BP_DIR}/BP_RiftGenerator",
        f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBattery",
        f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFloodlight",
    ]
    for path in paths:
        cdo = rw.blueprint_cdo(path)
        if not cdo:
            continue
        for prop in ["beacon_light", "status_light", "work_light"]:
            try:
                light = cdo.get_editor_property(prop)
                if light:
                    rw.safe_set(light, "volumetric_scattering_intensity", 0.08)
            except Exception:
                pass
        rw.asset_library.save_asset(path, only_if_is_dirty=False)

    for name in ["BP_RiftBreachRepulsion", "BP_RiftBreachAttraction", "BP_RiftBreachLuminance", "BP_RiftBreachGravity"]:
        path = f"{rw.GAMEPLAY_BP_DIR}/{name}"
        cdo = rw.blueprint_cdo(path)
        if not cdo:
            continue
        try:
            light = cdo.get_editor_property("core_light")
            rw.safe_set(light, "volumetric_scattering_intensity", 0.22 if name == "BP_RiftBreachLuminance" else 0.10)
        except Exception:
            pass
        rw.asset_library.save_asset(path, only_if_is_dirty=False)


def configure_mannequin_colossus() -> str:
    path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftMannequinColossus"
    rw.create_blueprint("BP_RiftMannequinColossus", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftMannequinColossus")
    cdo = rw.blueprint_cdo(path)
    mesh, skeleton, animations, score = rw.runtime_character_assets()
    if cdo and mesh and animations:
        try:
            if not rw.safe_set(cdo.get_editor_property("mesh"), "skeletal_mesh_asset", mesh):
                rw.safe_set(cdo.get_editor_property("mesh"), "skeletal_mesh", mesh)
        except Exception:
            pass
        idle = _pick(animations, ["Idle_Loop", "Idle"])
        walk = _pick(animations, ["Walking", "Walk_Loop", "Fast Run", "Jog"])
        if idle:
            rw.safe_set(cdo, "idle_animation", idle)
        if walk:
            rw.safe_set(cdo, "walk_animation", walk)
        rw.safe_set(cdo, "visual_scale", 16.0)
        rw.safe_set(cdo, "move_speed", 115.0)
        rw.asset_library.save_asset(path, only_if_is_dirty=False)
        log(f"Colossus uses Skeleton-native mesh={mesh.get_name()} clips={score}")
    return path


def replace_colossus_in_bootstrap(colossus_path: str) -> None:
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)
    existing = None
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label() == "RIFT_AUTO_WalkerColossus":
                existing = actor
                break
        except Exception:
            pass
    location = unreal.Vector(7900.0, -6500.0, 0.0)
    rotation = rw.rotator()
    if existing:
        try:
            location = existing.get_actor_location()
            rotation = existing.get_actor_rotation()
        except Exception:
            pass
        actors.destroy_actor(existing)
    cls = rw.blueprint_class(colossus_path)
    if cls:
        new_actor = actors.spawn_actor_from_class(cls, location, rotation)
        if new_actor:
            new_actor.set_actor_label("RIFT_AUTO_WalkerColossus")
            log("Replaced block Walker with skeleton-safe animated Colossus")
    level.save_current_level()


def apply_all() -> None:
    refresh_character_animation_defaults()
    reduce_legacy_volumetrics()
    colossus_path = configure_mannequin_colossus()
    replace_colossus_in_bootstrap(colossus_path)
    try:
        rw.asset_library.save_directory(rw.ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    log("Polish pass complete")


if __name__ == "__main__":
    apply_all()
