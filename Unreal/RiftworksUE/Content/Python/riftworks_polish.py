from __future__ import annotations

import unreal
import riftworks_setup as rw


def log(msg: str) -> None:
    unreal.log(f"[RIFTWORKS POLISH] {msg}")


def preferred_anim(names: list[str]):
    return rw.find_named_animation(names)


def refresh_character_animation_defaults() -> None:
    player_path = f"{rw.BP_DIR}/BP_RiftPlayer"
    npc_path = f"{rw.BP_DIR}/BP_RiftHumanoid"

    idle = preferred_anim(["Idle_Loop", "Idle"])
    walk = preferred_anim(["Walking", "Walk_Loop", "Walk"])
    run = preferred_anim(["Fast Run", "Sprint_Loop", "Jog_Fwd_Loop", "Jog"])
    crouch = preferred_anim(["Crouch_Fwd_Loop", "Crouch_Idle_Loop"])
    pistol_idle = preferred_anim(["Pistol_Idle_Loop", "Pistol Idle", "Idle_Loop"])
    pistol_shoot = preferred_anim(["Pistol_Shoot", "Pistol Shoot"])
    hit = preferred_anim(["Hit_Chest", "Hit Chest", "Hit_Head"])
    death = preferred_anim(["Death01", "Death"])

    player = rw.blueprint_cdo(player_path)
    if player:
        rw.safe_set(player, "idle_animation", idle)
        rw.safe_set(player, "walk_animation", walk)
        rw.safe_set(player, "run_animation", run)
        rw.safe_set(player, "crouch_animation", crouch)
        rw.safe_set(player, "pistol_shoot_animation", pistol_shoot)
        # Explicitly clear the old analytic cookie. The current flashlight uses a single clean spotlight.
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
        rw.safe_set(npc, "idle_animation", idle)
        rw.safe_set(npc, "walk_animation", walk)
        rw.safe_set(npc, "run_animation", run)
        rw.safe_set(npc, "pistol_idle_animation", pistol_idle)
        rw.safe_set(npc, "pistol_shoot_animation", pistol_shoot)
        rw.safe_set(npc, "hit_animation", hit)
        rw.safe_set(npc, "death_animation", death)
        rw.safe_set(npc, "health", 50.0)
        rw.asset_library.save_asset(npc_path, only_if_is_dirty=False)

    log(
        "Animation refresh: "
        f"idle={getattr(idle, 'get_name', lambda: None)()} "
        f"walk={getattr(walk, 'get_name', lambda: None)()} "
        f"run={getattr(run, 'get_name', lambda: None)()} "
        f"shoot={getattr(pistol_shoot, 'get_name', lambda: None)()}"
    )


def configure_mannequin_colossus() -> str:
    path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftMannequinColossus"
    rw.create_blueprint(
        "BP_RiftMannequinColossus",
        rw.GAMEPLAY_BP_DIR,
        "/Script/RiftworksUE.RiftMannequinColossus",
    )

    cdo = rw.blueprint_cdo(path)
    meshes = rw.find_by_type(rw.CHAR_DIR, unreal.SkeletalMesh)
    mesh = meshes[0] if meshes else None
    if cdo and mesh:
        try:
            rw.set_skeletal_mesh(cdo.get_editor_property("mesh"), mesh)
        except Exception:
            pass
        rw.safe_set(cdo, "idle_animation", preferred_anim(["Idle_Loop", "Idle"]))
        rw.safe_set(cdo, "walk_animation", preferred_anim(["Walking", "Walk_Loop", "Fast Run"]))
        rw.safe_set(cdo, "visual_scale", 16.0)
        rw.safe_set(cdo, "move_speed", 115.0)
        rw.asset_library.save_asset(path, only_if_is_dirty=False)
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
    rotation = unreal.Rotator()
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
            log("Replaced block Walker with animated mannequin Colossus")
    level.save_current_level()


def apply_all() -> None:
    refresh_character_animation_defaults()
    colossus_path = configure_mannequin_colossus()
    replace_colossus_in_bootstrap(colossus_path)
    try:
        rw.asset_library.save_directory(rw.ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    log("Polish pass complete")


if __name__ == "__main__":
    apply_all()
