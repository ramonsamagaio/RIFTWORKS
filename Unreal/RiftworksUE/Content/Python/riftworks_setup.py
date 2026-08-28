# RIFTWORKS Unreal 5.8 editor bootstrap.
# Run from Tools > Execute Python Script if automatic first-open setup was skipped.

from __future__ import annotations

import os
import re
import traceback
from typing import Iterable, Optional

import unreal

PROJECT_DIR = os.path.abspath(unreal.Paths.project_dir())
REPO_ROOT = os.path.abspath(os.path.join(PROJECT_DIR, "..", ".."))
SOURCE_ANIMS = os.path.join(REPO_ROOT, "assets", "anims")
SOURCE_MODELS = os.path.join(REPO_ROOT, "assets", "models", "Female Mannequin")

ROOT = "/Game/Riftworks"
CHAR_DIR = f"{ROOT}/Characters"
ANIM_DIR = f"{ROOT}/Animations"
EXTRA_ANIM_DIR = f"{ANIM_DIR}/Extras"
BP_DIR = f"{ROOT}/Blueprints"
SYSTEM_BP_DIR = f"{BP_DIR}/Systems"
GAMEPLAY_BP_DIR = f"{BP_DIR}/Gameplay"
MAT_DIR = f"{ROOT}/Materials"
MAP_DIR = f"{ROOT}/Maps"
BOOTSTRAP_MAP = f"{MAP_DIR}/L_RiftworksBootstrap"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_library = unreal.EditorAssetLibrary


def log(message: str) -> None:
    unreal.log(f"[RIFTWORKS SETUP] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[RIFTWORKS SETUP] {message}")


def safe_set(obj, prop: str, value) -> bool:
    if obj is None:
        return False
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception:
        return False


def ensure_dirs() -> None:
    for path in [ROOT, CHAR_DIR, ANIM_DIR, EXTRA_ANIM_DIR, BP_DIR, SYSTEM_BP_DIR, GAMEPLAY_BP_DIR, MAT_DIR, MAP_DIR]:
        if not asset_library.does_directory_exist(path):
            asset_library.make_directory(path)


def list_assets(folder: str) -> list:
    if not asset_library.does_directory_exist(folder):
        return []
    assets = []
    for path in asset_library.list_assets(folder, recursive=True, include_folder=False):
        try:
            asset = asset_library.load_asset(path)
            if asset:
                assets.append(asset)
        except Exception:
            pass
    return assets


def normalize(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.lower())


def find_by_type(folder: str, unreal_type) -> list:
    return [asset for asset in list_assets(folder) if isinstance(asset, unreal_type)]


def find_named_animation(candidates: Iterable[str]):
    animations = find_by_type(ANIM_DIR, unreal.AnimSequence)
    if not animations:
        return None
    normalized = [(asset, normalize(asset.get_name())) for asset in animations]
    for candidate in candidates:
        key = normalize(candidate)
        for asset, name in normalized:
            if name == key or key in name or name in key:
                return asset
    return None


def import_default(filename: str, destination: str) -> list[str]:
    if not os.path.isfile(filename):
        warn(f"Missing source asset: {filename}")
        return []
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.automated = True
    task.save = True
    task.replace_existing = False
    task.replace_existing_settings = False
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths or [])


def import_ual1() -> Optional[unreal.Skeleton]:
    existing = find_by_type(ANIM_DIR, unreal.Skeleton)
    if existing:
        log(f"Using existing animation skeleton: {existing[0].get_path_name()}")
        return existing[0]

    ual_glb = os.path.join(SOURCE_ANIMS, "UAL1_Standard.glb")
    log(f"Importing UAL1 animation library from {ual_glb}")
    import_default(ual_glb, ANIM_DIR)
    skeletons = find_by_type(ANIM_DIR, unreal.Skeleton)
    if not skeletons:
        warn("UAL1 imported without an exposed Skeleton. Female mannequin import will create one and extras will use it.")
        return None
    return skeletons[0]


def make_fbx_ui(skeleton=None, import_mesh=True, import_animations=False):
    ui = unreal.FbxImportUI()
    safe_set(ui, "automated_import_should_detect_type", False)
    safe_set(ui, "import_mesh", import_mesh)
    safe_set(ui, "import_animations", import_animations)
    safe_set(ui, "import_as_skeletal", import_mesh)
    if import_animations and not import_mesh:
        safe_set(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        safe_set(ui, "original_import_type", unreal.FBXImportType.FBXIT_ANIMATION)
    else:
        safe_set(ui, "mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        safe_set(ui, "original_import_type", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    if skeleton:
        safe_set(ui, "skeleton", skeleton)
    skeletal_data = None
    try:
        skeletal_data = ui.get_editor_property("skeletal_mesh_import_data")
    except Exception:
        pass
    if skeletal_data:
        safe_set(skeletal_data, "import_morph_targets", True)
        safe_set(skeletal_data, "update_skeleton_reference_pose", False)
        safe_set(skeletal_data, "use_t0_as_ref_pose", False)
    anim_data = None
    try:
        anim_data = ui.get_editor_property("anim_sequence_import_data")
    except Exception:
        pass
    if anim_data:
        safe_set(anim_data, "import_bone_tracks", True)
        safe_set(anim_data, "remove_redundant_keys", True)
    return ui


def import_fbx(filename: str, destination: str, options) -> list[str]:
    if not os.path.isfile(filename):
        warn(f"Missing FBX: {filename}")
        return []
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination
    task.automated = True
    task.save = True
    task.replace_existing = False
    task.replace_existing_settings = False
    task.options = options
    asset_tools.import_asset_tasks([task])
    return list(task.imported_object_paths or [])


def import_mannequin(shared_skeleton=None):
    existing = find_by_type(CHAR_DIR, unreal.SkeletalMesh)
    if existing:
        return existing[0], existing[0].get_editor_property("skeleton")

    # The repository includes a dedicated Unity FBX and Unreal/Godot GLB. FBX gives us explicit skeleton assignment.
    mannequin_fbx = os.path.join(SOURCE_MODELS, "Unity", "Mannequin_F.fbx")
    if os.path.isfile(mannequin_fbx):
        log("Importing Female Mannequin FBX onto the UAL1 skeleton")
        import_fbx(mannequin_fbx, CHAR_DIR, make_fbx_ui(shared_skeleton, True, False))
    else:
        mannequin_glb = os.path.join(SOURCE_MODELS, "Unreal-Godot", "Mannequin_F.glb")
        log("FBX not found; importing Female Mannequin GLB")
        import_default(mannequin_glb, CHAR_DIR)

    meshes = find_by_type(CHAR_DIR, unreal.SkeletalMesh)
    if not meshes:
        warn("Female mannequin import produced no SkeletalMesh.")
        return None, shared_skeleton
    mesh = meshes[0]
    skeleton = mesh.get_editor_property("skeleton") or shared_skeleton
    return mesh, skeleton


def import_extra_fbx_animations(skeleton) -> None:
    if not skeleton or not os.path.isdir(SOURCE_ANIMS):
        return
    for entry in sorted(os.listdir(SOURCE_ANIMS)):
        if not entry.lower().endswith(".fbx"):
            continue
        src = os.path.join(SOURCE_ANIMS, entry)
        base = os.path.splitext(entry)[0]
        if any(normalize(asset.get_name()) == normalize(base) for asset in list_assets(EXTRA_ANIM_DIR)):
            continue
        log(f"Importing extra animation: {entry}")
        import_fbx(src, EXTRA_ANIM_DIR, make_fbx_ui(skeleton, False, True))


def create_blueprint(name: str, folder: str, parent_class_path: str):
    path = f"{folder}/{name}"
    if asset_library.does_asset_exist(path):
        return asset_library.load_asset(path)
    parent = unreal.load_class(None, parent_class_path)
    if not parent:
        warn(f"Parent class unavailable: {parent_class_path}. Compile the C++ module, then run setup again.")
        return None
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp = asset_tools.create_asset(name, folder, unreal.Blueprint, factory)
    if bp:
        asset_library.save_asset(path, only_if_is_dirty=False)
        log(f"Created Blueprint {path}")
    return bp


def blueprint_class(path: str):
    try:
        return asset_library.load_blueprint_class(path)
    except Exception:
        return None


def blueprint_cdo(path: str):
    cls = blueprint_class(path)
    return unreal.get_default_object(cls) if cls else None


def set_skeletal_mesh(component, mesh) -> None:
    if not component or not mesh:
        return
    if not safe_set(component, "skeletal_mesh_asset", mesh):
        safe_set(component, "skeletal_mesh", mesh)
    try:
        component.set_relative_location(unreal.Vector(0.0, 0.0, -88.0))
        component.set_relative_rotation(unreal.Rotator(0.0, -90.0, 0.0))
    except Exception:
        pass


def create_light_function_material():
    asset_path = f"{MAT_DIR}/M_FlashlightCookie"
    if asset_library.does_asset_exist(asset_path):
        return asset_library.load_asset(asset_path)
    try:
        factory = unreal.MaterialFactoryNew()
        material = asset_tools.create_asset("M_FlashlightCookie", MAT_DIR, unreal.Material, factory)
        material.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
        uv = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -600, 0)
        center = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant2Vector, -600, 180)
        safe_set(center, "r", 0.5)
        safe_set(center, "g", 0.5)
        mask = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionSphereMask, -260, 40)
        safe_set(mask, "attenuation_radius", 0.43)
        safe_set(mask, "hardness_percent", 52.0)
        unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", mask, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(center, "", mask, "B")
        unreal.MaterialEditingLibrary.connect_material_property(mask, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(material)
        asset_library.save_asset(asset_path, only_if_is_dirty=False)
        log("Created analytic soft-edge flashlight light function")
        return material
    except Exception as exc:
        warn(f"Could not create light-function material automatically: {exc}")
        return None


def configure_character_blueprints(female_mesh) -> tuple:
    player_path = f"{BP_DIR}/BP_RiftPlayer"
    npc_path = f"{BP_DIR}/BP_RiftHumanoid"
    gm_path = f"{BP_DIR}/BP_RiftGameMode"
    world_path = f"{SYSTEM_BP_DIR}/BP_RiftWorldDirector"

    create_blueprint("BP_RiftPlayer", BP_DIR, "/Script/RiftworksUE.RiftPlayerCharacter")
    create_blueprint("BP_RiftHumanoid", BP_DIR, "/Script/RiftworksUE.RiftHumanoidNPC")
    create_blueprint("BP_RiftGameMode", BP_DIR, "/Script/RiftworksUE.RiftGameMode")
    create_blueprint("BP_RiftWorldDirector", SYSTEM_BP_DIR, "/Script/RiftworksUE.RiftWorldDirector")

    idle = find_named_animation(["Idle_Loop", "Idle"])
    walk = find_named_animation(["Walk_Loop", "Walking", "Walk"])
    run = find_named_animation(["Sprint_Loop", "Jog_Fwd_Loop", "Fast Run", "Jog"])
    crouch = find_named_animation(["Crouch_Fwd_Loop", "Crouch_Idle_Loop"])
    pistol_idle = find_named_animation(["Pistol_Idle_Loop", "Pistol Idle"])
    pistol_shoot = find_named_animation(["Pistol_Shoot", "Pistol Shoot"])
    death = find_named_animation(["Death01", "Death"])

    player_cdo = blueprint_cdo(player_path)
    if player_cdo:
        set_skeletal_mesh(player_cdo.get_editor_property("mesh"), female_mesh)
        safe_set(player_cdo, "idle_animation", idle)
        safe_set(player_cdo, "walk_animation", walk)
        safe_set(player_cdo, "run_animation", run)
        safe_set(player_cdo, "crouch_animation", crouch)
        safe_set(player_cdo, "pistol_shoot_animation", pistol_shoot)
        safe_set(player_cdo, "scrap", 24)
        cookie = create_light_function_material()
        try:
            flashlight = player_cdo.get_editor_property("flashlight")
            if cookie:
                safe_set(flashlight, "light_function_material", cookie)
        except Exception:
            pass
        asset_library.save_asset(player_path, only_if_is_dirty=False)

    npc_cdo = blueprint_cdo(npc_path)
    if npc_cdo:
        set_skeletal_mesh(npc_cdo.get_editor_property("mesh"), female_mesh)
        safe_set(npc_cdo, "idle_animation", idle)
        safe_set(npc_cdo, "walk_animation", walk)
        safe_set(npc_cdo, "run_animation", run)
        safe_set(npc_cdo, "pistol_idle_animation", pistol_idle or idle)
        safe_set(npc_cdo, "pistol_shoot_animation", pistol_shoot)
        safe_set(npc_cdo, "death_animation", death)
        asset_library.save_asset(npc_path, only_if_is_dirty=False)

    gm_cdo = blueprint_cdo(gm_path)
    player_cls = blueprint_class(player_path)
    if gm_cdo and player_cls:
        safe_set(gm_cdo, "default_pawn_class", player_cls)
        asset_library.save_asset(gm_path, only_if_is_dirty=False)

    return blueprint_class(player_path), blueprint_class(npc_path), blueprint_class(gm_path), blueprint_class(world_path)


def enum_value(enum_type_name: str, value_name: str):
    enum_type = getattr(unreal, enum_type_name, None)
    if not enum_type:
        return None
    for candidate in [value_name.upper(), value_name, value_name.title().replace("_", "")]:
        if hasattr(enum_type, candidate):
            return getattr(enum_type, candidate)
    return None


def create_gameplay_blueprints() -> dict[str, str]:
    definitions = {
        "BP_RiftBaseBeacon": "/Script/RiftworksUE.RiftBaseBeacon",
        "BP_RiftSalvage": "/Script/RiftworksUE.RiftSalvageActor",
        "BP_RiftGenerator": "/Script/RiftworksUE.RiftPowerDevice",
        "BP_RiftBattery": "/Script/RiftworksUE.RiftPowerDevice",
        "BP_RiftFloodlight": "/Script/RiftworksUE.RiftPowerDevice",
        "BP_RiftBreachRepulsion": "/Script/RiftworksUE.RiftBreachEmitter",
        "BP_RiftBreachAttraction": "/Script/RiftworksUE.RiftBreachEmitter",
        "BP_RiftBreachLuminance": "/Script/RiftworksUE.RiftBreachEmitter",
        "BP_RiftBreachGravity": "/Script/RiftworksUE.RiftBreachEmitter",
        "BP_RiftFASPlatform": "/Script/RiftworksUE.RiftAssemblyPart",
        "BP_RiftFASBeam": "/Script/RiftworksUE.RiftAssemblyPart",
        "BP_RiftFASWheel": "/Script/RiftworksUE.RiftAssemblyPart",
        "BP_RiftFASMotorWheel": "/Script/RiftworksUE.RiftAssemblyPart",
        "BP_RiftColossus": "/Script/RiftworksUE.RiftColossus",
        "BP_RiftHarpoon": "/Script/RiftworksUE.RiftHarpoonAnchor",
    }
    paths = {}
    for name, parent in definitions.items():
        create_blueprint(name, GAMEPLAY_BP_DIR, parent)
        paths[name] = f"{GAMEPLAY_BP_DIR}/{name}"

    power_enum = getattr(unreal, "RiftPowerKind", None)
    if power_enum:
        for name, enum_name, settings in [
            ("BP_RiftGenerator", "GENERATOR", {"generation_kw": 3.2, "device_name": "Portable Generator"}),
            ("BP_RiftBattery", "BATTERY", {"capacity_k_wh": 5.0, "charge_k_wh": 3.8, "device_name": "Battery Bank"}),
            ("BP_RiftFloodlight", "CONSUMER", {"consumption_kw": 0.65, "device_name": "Field Floodlight"}),
        ]:
            cdo = blueprint_cdo(paths[name])
            value = getattr(power_enum, enum_name, None)
            if cdo and value is not None:
                safe_set(cdo, "kind", value)
                for prop, setting in settings.items():
                    safe_set(cdo, prop, setting)
                asset_library.save_asset(paths[name], only_if_is_dirty=False)

    breach_enum = getattr(unreal, "RiftBreachMode", None)
    if breach_enum:
        for name, enum_name in [
            ("BP_RiftBreachRepulsion", "REPULSION"),
            ("BP_RiftBreachAttraction", "ATTRACTION"),
            ("BP_RiftBreachLuminance", "LUMINANCE"),
            ("BP_RiftBreachGravity", "GRAVITY"),
        ]:
            cdo = blueprint_cdo(paths[name])
            value = getattr(breach_enum, enum_name, None)
            if cdo and value is not None:
                safe_set(cdo, "mode", value)
                asset_library.save_asset(paths[name], only_if_is_dirty=False)

    part_enum = getattr(unreal, "RiftAssemblyPartType", None)
    if part_enum:
        for name, enum_name in [
            ("BP_RiftFASPlatform", "PLATFORM"),
            ("BP_RiftFASBeam", "BEAM"),
            ("BP_RiftFASWheel", "WHEEL"),
            ("BP_RiftFASMotorWheel", "MOTOR_WHEEL"),
        ]:
            cdo = blueprint_cdo(paths[name])
            value = getattr(part_enum, enum_name, None)
            if value is None and enum_name == "MOTOR_WHEEL":
                value = getattr(part_enum, "MOTORWHEEL", None)
            if cdo and value is not None:
                safe_set(cdo, "part_type", value)
                asset_library.save_asset(paths[name], only_if_is_dirty=False)

    salvage_cdo = blueprint_cdo(paths["BP_RiftSalvage"])
    if salvage_cdo:
        safe_set(salvage_cdo, "display_name", "Salvage")
        asset_library.save_asset(paths["BP_RiftSalvage"], only_if_is_dirty=False)

    return paths


def spawn_bp(actor_subsystem, bp_path: str, location, rotation=None):
    cls = blueprint_class(bp_path)
    if not cls:
        return None
    rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    try:
        return actor_subsystem.spawn_actor_from_class(cls, location, rotation)
    except Exception as exc:
        warn(f"Could not spawn {bp_path}: {exc}")
        return None


def configure_power_instance(actor, kind: str) -> None:
    power_enum = getattr(unreal, "RiftPowerKind", None)
    if not actor or not power_enum:
        return
    value = getattr(power_enum, kind.upper(), None)
    if value is not None:
        safe_set(actor, "kind", value)


def build_bootstrap_map(npc_class, gm_class, world_class, gameplay_paths) -> None:
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if asset_library.does_asset_exist(BOOTSTRAP_MAP):
        level_subsystem.load_level(BOOTSTRAP_MAP)
    else:
        log("Creating L_RiftworksBootstrap")
        if not level_subsystem.new_level(BOOTSTRAP_MAP):
            warn("Could not create bootstrap map automatically.")
            return

    # Remove only actors placed by this bootstrap so repeated runs are idempotent.
    for actor in list(actor_subsystem.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            label = ""
        if label.startswith("RIFT_AUTO_"):
            actor_subsystem.destroy_actor(actor)

    def label(actor, name):
        if actor:
            try:
                actor.set_actor_label(f"RIFT_AUTO_{name}")
            except Exception:
                pass
        return actor

    if world_class:
        label(actor_subsystem.spawn_actor_from_class(world_class, unreal.Vector(0, 0, 0), unreal.Rotator()), "WorldDirector")

    try:
        player_start = actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 180), unreal.Rotator())
        label(player_start, "PlayerStart")
    except Exception as exc:
        warn(f"PlayerStart spawn failed: {exc}")

    base = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftBaseBeacon"], unreal.Vector(260, 3800, 90)), "StarterBase")
    generator = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftGenerator"], unreal.Vector(-260, 3650, 80)), "Generator")
    battery = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftBattery"], unreal.Vector(0, 3650, 80)), "Battery")
    light = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftFloodlight"], unreal.Vector(330, 3550, 80), unreal.Rotator(0, 180, 0)), "Floodlight")
    configure_power_instance(generator, "GENERATOR")
    configure_power_instance(battery, "BATTERY")
    configure_power_instance(light, "CONSUMER")
    if generator:
        safe_set(generator, "generation_kw", 3.2)
    if battery:
        safe_set(battery, "capacity_k_wh", 5.0)
        safe_set(battery, "charge_k_wh", 3.8)
    if light:
        safe_set(light, "consumption_kw", 0.65)
    try:
        if generator and battery:
            generator.connect_to(battery)
        if battery and light:
            battery.connect_to(light)
    except Exception as exc:
        warn(f"Starter grid linking failed: {exc}")

    if npc_class:
        npc_positions = [
            unreal.Vector(2800, -3400, 120), unreal.Vector(-1600, -4900, 120),
            unreal.Vector(5100, 1800, 120), unreal.Vector(-5800, 5100, 120),
            unreal.Vector(1800, 6400, 120),
        ]
        for index, pos in enumerate(npc_positions):
            label(actor_subsystem.spawn_actor_from_class(npc_class, pos, unreal.Rotator()), f"Humanoid_{index:02d}")

    colossus = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftColossus"], unreal.Vector(7900, -6500, 0)), "WalkerColossus")
    harpoon = label(spawn_bp(actor_subsystem, gameplay_paths["BP_RiftHarpoon"], unreal.Vector(4400, -4200, 70), unreal.Rotator(0, -45, 0)), "Harpoon")

    salvage_cls = blueprint_class(gameplay_paths["BP_RiftSalvage"])
    if salvage_cls:
        salvage_specs = [
            (unreal.Vector(700, 4100, 70), "scrap", "Machined Scrap", 4, False, 2.0),
            (unreal.Vector(1100, 3600, 70), "electronics", "Control Board", 1, False, 1.0),
            (unreal.Vector(1650, 3300, 70), "motor", "Industrial Motor", 1, True, 28.0),
            (unreal.Vector(-900, 4200, 70), "cable", "Cable Coil", 2, False, 1.0),
            (unreal.Vector(-1300, 3300, 70), "fuel", "Fuel Can", 1, False, 3.0),
        ]
        for index, (pos, item, display, amount, heavy, mass) in enumerate(salvage_specs):
            actor = actor_subsystem.spawn_actor_from_class(salvage_cls, pos, unreal.Rotator())
            label(actor, f"Salvage_{index:02d}_{item}")
            safe_set(actor, "item_id", item)
            safe_set(actor, "display_name", display)
            safe_set(actor, "amount", amount)
            safe_set(actor, "b_heavy", heavy)
            safe_set(actor, "mass_kg", mass)
            try:
                actor.set_carried_state(False)
            except Exception:
                pass

    # Nav volume for animated humanoid patrol/chase. Runtime generation is enabled in config.
    try:
        nav = actor_subsystem.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 300), unreal.Rotator())
        if nav:
            nav.set_actor_scale3d(unreal.Vector(90.0, 90.0, 18.0))
            label(nav, "NavMeshBounds")
    except Exception as exc:
        warn(f"NavMeshBoundsVolume could not be spawned automatically: {exc}")

    try:
        editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world_settings = editor_world.get_world_settings() if editor_world else None
        if world_settings and gm_class:
            safe_set(world_settings, "default_game_mode", gm_class)
    except Exception as exc:
        warn(f"Could not set WorldSettings GameMode override: {exc}")

    level_subsystem.save_current_level()
    log("Bootstrap level saved")


def setup_all(force_level_refresh: bool = True) -> None:
    log(f"Project: {PROJECT_DIR}")
    log(f"Repository root: {REPO_ROOT}")
    ensure_dirs()

    shared_skeleton = import_ual1()
    female_mesh, female_skeleton = import_mannequin(shared_skeleton)
    skeleton = shared_skeleton or female_skeleton
    import_extra_fbx_animations(skeleton)

    player_class, npc_class, gm_class, world_class = configure_character_blueprints(female_mesh)
    gameplay_paths = create_gameplay_blueprints()

    if force_level_refresh:
        build_bootstrap_map(npc_class, gm_class, world_class, gameplay_paths)

    try:
        asset_library.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    log("SETUP COMPLETE. Player/NPC Blueprints use the shared mannequin animation library; open L_RiftworksBootstrap and Play.")


if __name__ == "__main__":
    try:
        setup_all(True)
    except Exception:
        unreal.log_error("[RIFTWORKS SETUP] Fatal bootstrap error:\n" + traceback.format_exc())
