from __future__ import annotations

import re
import unreal
import riftworks_setup as rw
import riftworks_visuals as rv


CHARACTER_BP_PATHS = [
    f"{rw.BP_DIR}/BP_RiftPlayer",
    f"{rw.BP_DIR}/BP_RiftProductionPlayer",
    f"{rw.BP_DIR}/BP_RiftEngineeringPlayer",
    f"{rw.BP_DIR}/BP_RiftHumanoid",
]
COLOSSUS_BP_PATH = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftMannequinColossus"


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
            rw.safe_set(spot, "volumetric_scattering_intensity", 0.04)
            if "work" in spot.get_name().lower():
                rw.safe_set(spot, "intensity", 3400.0)
                rw.safe_set(spot, "attenuation_radius", 2600.0)
        except Exception:
            pass
    for point in _all_components(actor, unreal.PointLightComponent):
        try:
            name = point.get_name().lower()
            rw.safe_set(point, "volumetric_scattering_intensity", 0.03 if "core" not in name else 0.08)
        except Exception:
            pass


def _asset_skeleton(asset):
    if not asset:
        return None
    try:
        return asset.get_editor_property("skeleton")
    except Exception:
        return None


def _same_asset(a, b) -> bool:
    if not a or not b:
        return False
    try:
        return a.get_path_name() == b.get_path_name()
    except Exception:
        return a == b


def _normalize_name(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.lower())


def _runtime_character_assets():
    """Choose the mesh that actually owns the animation-library skeleton.

    The Female Mannequin is a retarget target. It must not be force-bound to the
    UAL1 Skeleton asset merely because the source rigs have matching bone names.
    Until an IK Retargeter asset is authored, runtime characters use the mesh
    imported with UAL1 so every animation is skeleton-native and cannot T-pose.
    """
    animations = rw.find_by_type(rw.ANIM_DIR, unreal.AnimSequence)
    meshes = rw.find_by_type(rw.ANIM_DIR, unreal.SkeletalMesh)

    best_mesh = None
    best_skeleton = None
    best_score = -1
    for mesh in meshes:
        skeleton = _asset_skeleton(mesh)
        if not skeleton:
            continue
        score = sum(1 for anim in animations if _same_asset(_asset_skeleton(anim), skeleton))
        if score > best_score:
            best_mesh = mesh
            best_skeleton = skeleton
            best_score = score

    if not best_mesh:
        # Last-resort compatibility search. Never choose a mesh whose Skeleton is
        # unrelated to every imported animation.
        for mesh in rw.find_by_type(rw.CHAR_DIR, unreal.SkeletalMesh):
            skeleton = _asset_skeleton(mesh)
            score = sum(1 for anim in animations if _same_asset(_asset_skeleton(anim), skeleton))
            if score > best_score and score > 0:
                best_mesh = mesh
                best_skeleton = skeleton
                best_score = score

    compatible = [a for a in animations if _same_asset(_asset_skeleton(a), best_skeleton)] if best_skeleton else []
    return best_mesh, best_skeleton, compatible, best_score


def _pick_animation(animations, candidates):
    normalized = [(asset, _normalize_name(asset.get_name())) for asset in animations]
    for candidate in candidates:
        key = _normalize_name(candidate)
        for asset, name in normalized:
            if name == key or key in name or name in key:
                return asset
    return None


def _mesh_floor_offset(mesh, capsule_half_height=92.0) -> float:
    # Imported FBX/GLB assets do not all share the same pivot. Compute the
    # placement from the actual local bounds instead of assuming -88/-90 UE
    # mannequin defaults.
    try:
        bounds = mesh.get_bounds()
        bottom = float(bounds.origin.z - bounds.box_extent.z)
        offset = -float(capsule_half_height) - bottom
        if -300.0 <= offset <= 300.0:
            return offset
    except Exception:
        pass
    return -float(capsule_half_height)


def _assign_mesh_component(component, mesh, capsule_half_height=92.0, scale=1.0):
    if not component or not mesh:
        return
    if not rw.safe_set(component, "skeletal_mesh_asset", mesh):
        rw.safe_set(component, "skeletal_mesh", mesh)
    try:
        component.set_relative_location(unreal.Vector(0.0, 0.0, _mesh_floor_offset(mesh, capsule_half_height)))
        component.set_relative_rotation(unreal.Rotator(0.0, 0.0, 0.0))
        component.set_relative_scale3d(unreal.Vector(scale, scale, scale))
    except Exception:
        pass


def _sanitize_character_blueprints(mesh, animations):
    if not mesh or not animations:
        rw.warn("Character audit found no skeleton-native UAL1 mesh/animation pair; leaving current assets untouched")
        return

    clips = {
        "idle_animation": _pick_animation(animations, ["Idle_Loop", "Idle"]),
        "walk_animation": _pick_animation(animations, ["Walk_Loop", "Walking", "Walk"]),
        "run_animation": _pick_animation(animations, ["Sprint_Loop", "Jog_Fwd_Loop", "Fast Run", "Jog"]),
        "crouch_animation": _pick_animation(animations, ["Crouch_Fwd_Loop", "Crouch_Idle_Loop"]),
        "pistol_idle_animation": _pick_animation(animations, ["Pistol_Idle_Loop", "Pistol Idle", "Idle_Loop"]),
        "pistol_shoot_animation": _pick_animation(animations, ["Pistol_Shoot", "Pistol Shoot"]),
        "hit_animation": _pick_animation(animations, ["Hit_Chest", "Hit_Head", "Hit"]),
        "death_animation": _pick_animation(animations, ["Death01", "Death"]),
    }

    for path in CHARACTER_BP_PATHS:
        cdo = rw.blueprint_cdo(path)
        if not cdo:
            continue
        try:
            _assign_mesh_component(cdo.get_editor_property("mesh"), mesh, 92.0, 1.0)
        except Exception:
            pass
        for prop, clip in clips.items():
            if clip:
                rw.safe_set(cdo, prop, clip)
        try:
            rw.asset_library.save_asset(path, only_if_is_dirty=False)
        except Exception:
            pass

    colossus = rw.blueprint_cdo(COLOSSUS_BP_PATH)
    if colossus:
        try:
            comp = colossus.get_editor_property("mesh")
            if not rw.safe_set(comp, "skeletal_mesh_asset", mesh):
                rw.safe_set(comp, "skeletal_mesh", mesh)
        except Exception:
            pass
        if clips["idle_animation"]:
            rw.safe_set(colossus, "idle_animation", clips["idle_animation"])
        if clips["walk_animation"]:
            rw.safe_set(colossus, "walk_animation", clips["walk_animation"])
        try:
            rw.asset_library.save_asset(COLOSSUS_BP_PATH, only_if_is_dirty=False)
        except Exception:
            pass


def _desired_floor_z(y: float):
    # Only flat staging bands. Sloped stair corridors are deliberately skipped.
    if y > -4200.0:
        return 24.0
    if -8350.0 < y <= -6900.0:
        return -901.0
    if y <= -9300.0:
        return -1320.0
    return None


def _ground_staged_actor(actor) -> bool:
    try:
        location = actor.get_actor_location()
        floor_z = _desired_floor_z(float(location.y))
        if floor_z is None:
            return False
        origin, extent = actor.get_actor_bounds(False)
        bottom = float(origin.z - extent.z)
        delta = floor_z - bottom
        # Large differences usually mean a deliberately elevated prop or bad
        # bounds. Do not bulldoze authored landmarks during the audit.
        if 2.0 < abs(delta) <= 360.0:
            actor.set_actor_location(unreal.Vector(location.x, location.y, location.z + delta), False, False)
            return True
    except Exception:
        pass
    return False


def _is_grounded_gameplay_actor(actor, label: str) -> bool:
    try:
        cls = actor.get_class().get_name().lower()
    except Exception:
        cls = ""
    label_l = label.lower()
    tokens = (
        "rifthumanoid", "riftsalvage", "riftpowerdevice", "riftbasebeacon",
        "riftoutpost", "riftcargocart", "riftconveyor", "riftfreightlift",
        "riftrecoverywinch", "riftfabricator", "riftlogicnode", "riftcrawler",
        "riftbreachgolem",
    )
    return any(token in cls for token in tokens) or any(token in label_l for token in (
        "humanoid", "salvage", "cargo", "generator", "battery", "floodlight",
        "starterbase", "outpost", "fabricator", "crawler",
    ))


def _sanitize_orientation(actor, label: str):
    try:
        rotation = actor.get_actor_rotation()
    except Exception:
        return

    # Preserve player-authored engineering transforms. Only normalize staged
    # gameplay actors and known primitive props generated by our own scripts.
    if _is_grounded_gameplay_actor(actor, label):
        try:
            actor.set_actor_rotation(unreal.Rotator(0.0, rotation.yaw, 0.0), False)
        except Exception:
            pass

    if "_Wheel_" in label or "DumpsterWheel" in label:
        try:
            actor.set_actor_rotation(unreal.Rotator(0.0, rotation.yaw, 90.0), False)
        except Exception:
            pass
    elif "UG_PipeRed" in label:
        try:
            actor.set_actor_rotation(unreal.Rotator(0.0, rotation.yaw, 90.0), False)
        except Exception:
            pass


def _sanitize_level_instances(mesh):
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return (0, 0)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    grounded = 0
    oriented = 0
    mats = rv.ensure_material_library()

    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""

        if label == "RIFT_AUTO_WalkerColossus":
            if mesh:
                for comp in _all_components(actor, unreal.SkeletalMeshComponent):
                    if not rw.safe_set(comp, "skeletal_mesh_asset", mesh):
                        rw.safe_set(comp, "skeletal_mesh", mesh)
            _set_skeletal_material(actor, mats.get("colossus"))
            _quiet_lights(actor)
        elif label.startswith("RIFT_AUTO_Humanoid"):
            if mesh:
                for comp in _all_components(actor, unreal.SkeletalMeshComponent):
                    _assign_mesh_component(comp, mesh, 92.0, 1.0)
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

        before = None
        try:
            before = actor.get_actor_rotation()
        except Exception:
            pass
        _sanitize_orientation(actor, label)
        if before is not None:
            try:
                after = actor.get_actor_rotation()
                if abs(after.pitch - before.pitch) > 0.1 or abs(after.roll - before.roll) > 0.1:
                    oriented += 1
            except Exception:
                pass
        if _is_grounded_gameplay_actor(actor, label) and label != "RIFT_AUTO_WalkerColossus":
            grounded += 1 if _ground_staged_actor(actor) else 0

    level.save_current_level()
    return grounded, oriented


def apply_all():
    mesh, skeleton, animations, score = _runtime_character_assets()
    _sanitize_character_blueprints(mesh, animations)
    grounded, oriented = _sanitize_level_instances(mesh)

    mesh_name = mesh.get_path_name() if mesh else "NONE"
    skeleton_name = skeleton.get_path_name() if skeleton else "NONE"
    rw.log(
        "FINAL AUDIT complete | "
        f"runtime_mesh={mesh_name} | skeleton={skeleton_name} | native_clips={len(animations)} | "
        f"compatibility_score={score} | grounded={grounded} | reoriented={oriented}"
    )


if __name__ == "__main__":
    apply_all()
