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


def _rotator(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


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
    # setup.py owns the canonical selection rule. Keep a local fallback only for
    # projects opened before the latest setup module has reloaded.
    try:
        return rw.runtime_character_assets()
    except Exception:
        pass

    animations = rw.find_by_type(rw.ANIM_DIR, unreal.AnimSequence)
    meshes = rw.find_by_type(rw.ANIM_DIR, unreal.SkeletalMesh) + rw.find_by_type(rw.CHAR_DIR, unreal.SkeletalMesh)
    best_mesh = None
    best_skeleton = None
    best_score = 0
    for mesh in meshes:
        skeleton = _asset_skeleton(mesh)
        if not skeleton:
            continue
        score = sum(1 for anim in animations if _same_asset(_asset_skeleton(anim), skeleton))
        if score > best_score:
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


def _assign_mesh_component(component, mesh, capsule_half_height=92.0):
    if not component or not mesh:
        return
    try:
        rw.set_skeletal_mesh(component, mesh, capsule_half_height)
    except TypeError:
        rw.set_skeletal_mesh(component, mesh)
    except Exception:
        pass


def _sanitize_character_blueprints(mesh, animations):
    compatible = bool(mesh and animations)
    clips = {
        "idle_animation": _pick_animation(animations, ["Idle_Loop", "Idle"]) if compatible else None,
        "walk_animation": _pick_animation(animations, ["Walk_Loop", "Walking", "Walk"]) if compatible else None,
        "run_animation": _pick_animation(animations, ["Sprint_Loop", "Jog_Fwd_Loop", "Fast Run", "Jog"]) if compatible else None,
        "crouch_animation": _pick_animation(animations, ["Crouch_Fwd_Loop", "Crouch_Idle_Loop"]) if compatible else None,
        "pistol_idle_animation": _pick_animation(animations, ["Pistol_Idle_Loop", "Pistol Idle", "Idle_Loop"]) if compatible else None,
        "pistol_shoot_animation": _pick_animation(animations, ["Pistol_Shoot", "Pistol Shoot"]) if compatible else None,
        "hit_animation": _pick_animation(animations, ["Hit_Chest", "Hit_Head", "Hit"]) if compatible else None,
        "death_animation": _pick_animation(animations, ["Death01", "Death"]) if compatible else None,
    }

    for path in CHARACTER_BP_PATHS:
        cdo = rw.blueprint_cdo(path)
        if not cdo:
            continue
        if mesh:
            try:
                _assign_mesh_component(cdo.get_editor_property("mesh"), mesh, 92.0)
            except Exception:
                pass
        rw.safe_set(cdo, "b_use_single_node_animation_fallback", compatible)
        for prop, clip in clips.items():
            if clip:
                rw.safe_set(cdo, prop, clip)
        try:
            rw.asset_library.save_asset(path, only_if_is_dirty=False)
        except Exception:
            pass

    colossus = rw.blueprint_cdo(COLOSSUS_BP_PATH)
    if colossus and mesh:
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


def _inside(x, y, center_x, center_y, width, depth) -> bool:
    return abs(x - center_x) <= width * 0.5 and abs(y - center_y) <= depth * 0.5


def _surface_floor_z(x: float, y: float) -> float:
    # Curated vertical-slice surfaces. Use the actual top faces of the generated
    # geometry instead of a single guessed Z for the entire surface district.
    if _inside(x, y, 1550.0, 950.0, 1900.0, 1600.0):
        return 24.0  # Workshop floor
    if _inside(x, y, -1480.0, 1250.0, 1380.0, 1040.0):
        return 24.0  # Corner store floor
    if _inside(x, y, -1700.0, -1450.0, 1850.0, 1420.0):
        return 24.0  # Motel floor
    if _inside(x, y, 1650.0, -1750.0, 2000.0, 1500.0):
        return 16.0  # Substation pad
    if _inside(x, y, -620.0, -2190.0, 430.0, 390.0):
        return 20.0  # Checkpoint booth floor
    if abs(x) <= 460.0 and -3950.0 <= y <= 5050.0:
        return 8.0   # Main road
    if 465.0 <= abs(x) <= 675.0 and -3950.0 <= y <= 5050.0:
        return 24.0  # Sidewalk
    return 0.0       # Ground slabs


def _desired_floor_z(x: float, y: float):
    # Sloped stair bands are deliberately excluded because a single Z would be
    # less correct than their authored step geometry.
    if y > -4200.0:
        return _surface_floor_z(x, y)
    if -8150.0 <= y <= -5650.0:
        return -908.0   # Station hall floor: center -925, thickness 34
    if -8320.0 <= y < -8150.0:
        return -906.0   # Deep landing top
    if y <= -9400.0:
        return -1317.5  # Breach chamber floor top
    return None


def _ground_staged_actor(actor) -> bool:
    try:
        location = actor.get_actor_location()
        floor_z = _desired_floor_z(float(location.x), float(location.y))
        if floor_z is None:
            return False
        # only_colliding_components=True prevents light attenuation radii and
        # other visual helpers from becoming part of the actor's "feet".
        origin, extent = actor.get_actor_bounds(True)
        bottom = float(origin.z - extent.z)
        delta = float(floor_z) - bottom
        if 1.5 < abs(delta) <= 420.0:
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
    class_tokens = (
        "rifthumanoid", "riftsalvage", "riftpowerdevice", "riftbasebeacon",
        "riftoutpost", "riftcargocart", "riftconveyor", "riftfreightlift",
        "riftrecoverywinch", "riftfabricator", "riftlogicnode", "riftcrawler",
        "riftbreachgolem", "rifttemperaturefield", "riftphasefield",
    )
    label_tokens = (
        "humanoid", "salvage", "cargo", "generator", "battery", "floodlight",
        "starterbase", "outpost", "fabricator", "crawler", "winch", "recoveryrig",
    )
    return any(token in cls for token in class_tokens) or any(token in label_l for token in label_tokens)


def _sanitize_orientation(actor, label: str) -> bool:
    try:
        before = actor.get_actor_rotation()
    except Exception:
        return False

    # Grounded gameplay actors are intended to stand upright. Preserve heading,
    # never preserve accidental pitch/roll inherited from old Python staging.
    if _is_grounded_gameplay_actor(actor, label):
        try:
            actor.set_actor_rotation(_rotator(yaw=before.yaw), False)
        except Exception:
            pass

    # Engine BasicShapes Cylinder is Z-axis aligned. These known generated props
    # are intentionally horizontal, so normalize their roll explicitly.
    if "_Wheel_" in label or "DumpsterWheel" in label:
        try:
            current = actor.get_actor_rotation()
            actor.set_actor_rotation(_rotator(yaw=current.yaw, roll=90.0), False)
        except Exception:
            pass
    elif "UG_PipeRed" in label:
        try:
            current = actor.get_actor_rotation()
            actor.set_actor_rotation(_rotator(yaw=current.yaw, roll=90.0), False)
        except Exception:
            pass

    try:
        after = actor.get_actor_rotation()
        return (
            abs(after.pitch - before.pitch) > 0.1
            or abs(after.roll - before.roll) > 0.1
            or abs(after.yaw - before.yaw) > 0.1
        )
    except Exception:
        return False


def _sanitize_level_instances(mesh):
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return 0, 0

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)
    mats = rv.ensure_material_library()
    grounded = 0
    reoriented = 0

    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""

        # No blanket "legacy axis migration" here. Current art/dressing/weather
        # passes rebuild their own actors every editor open. Guessing intent from
        # an arbitrary old Euler rotation would risk corrupting freshly-correct
        # actors a second time.
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
                    _assign_mesh_component(comp, mesh, 92.0)
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

        reoriented += 1 if _sanitize_orientation(actor, label) else 0
        if _is_grounded_gameplay_actor(actor, label) and label != "RIFT_AUTO_WalkerColossus":
            grounded += 1 if _ground_staged_actor(actor) else 0

    level.save_current_level()
    return grounded, reoriented


def apply_all():
    mesh, skeleton, animations, score = _runtime_character_assets()
    _sanitize_character_blueprints(mesh, animations)
    grounded, reoriented = _sanitize_level_instances(mesh)

    mesh_name = mesh.get_path_name() if mesh else "NONE"
    skeleton_name = skeleton.get_path_name() if skeleton else "NONE"
    rw.log(
        "FINAL AUDIT complete | "
        f"runtime_mesh={mesh_name} | skeleton={skeleton_name} | native_clips={len(animations)} | "
        f"compatibility_score={score} | grounded={grounded} | reoriented={reoriented} | "
        "legacy_axis_guessing=disabled"
    )


if __name__ == "__main__":
    apply_all()
