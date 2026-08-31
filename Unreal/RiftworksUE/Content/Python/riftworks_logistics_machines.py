from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_GDD_LOGISTICS_"


def _rotator(pitch=0.0, yaw=0.0, roll=0.0):
    return rw.rotator(pitch=pitch, yaw=yaw, roll=roll)


def _mat(component, material):
    if component and material:
        try:
            component.set_material(0, material)
        except Exception:
            pass


def _ensure_blueprints(materials):
    conveyor_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftPoweredConveyor"
    lift_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFreightLift"
    cart_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftPhysicsCargoCart"

    rw.create_blueprint("BP_RiftPoweredConveyor", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftPoweredConveyor")
    rw.create_blueprint("BP_RiftFreightLift", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftFreightLift")
    rw.create_blueprint("BP_RiftPhysicsCargoCart", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftPhysicsCargoCart")

    conveyor = rw.blueprint_cdo(conveyor_path)
    if conveyor:
        try:
            _mat(conveyor.get_editor_property("mesh"), materials.get("metal"))
            _mat(conveyor.get_editor_property("belt"), materials.get("rubber"))
        except Exception:
            pass
        rw.asset_library.save_asset(conveyor_path, only_if_is_dirty=False)

    lift = rw.blueprint_cdo(lift_path)
    if lift:
        try:
            _mat(lift.get_editor_property("mesh"), materials.get("metal"))
            _mat(lift.get_editor_property("platform"), materials.get("assembly"))
            _mat(lift.get_editor_property("left_rail"), materials.get("rust"))
            _mat(lift.get_editor_property("right_rail"), materials.get("rust"))
        except Exception:
            pass
        rw.asset_library.save_asset(lift_path, only_if_is_dirty=False)

    cart = rw.blueprint_cdo(cart_path)
    if cart:
        try:
            _mat(cart.get_editor_property("deck"), materials.get("assembly"))
            _mat(cart.get_editor_property("handle"), materials.get("rust"))
        except Exception:
            pass
        rw.asset_library.save_asset(cart_path, only_if_is_dirty=False)
    return conveyor_path, lift_path, cart_path


def _spawn(actors, cls, label, location, rotation=(0.0, 0.0, 0.0)):
    pitch, yaw, roll = rotation
    actor = actors.spawn_actor_from_class(
        cls,
        unreal.Vector(*location),
        _rotator(pitch=pitch, yaw=yaw, roll=roll),
    )
    if actor:
        try:
            actor.set_actor_label(PREFIX + label)
        except Exception:
            pass
    return actor


def _connect(generator, device):
    if not generator or not device:
        return
    try:
        generator.connect_to(device)
    except Exception:
        pass


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    materials = rv.ensure_material_library()
    conveyor_path, lift_path, cart_path = _ensure_blueprints(materials)
    conveyor_cls = rw.blueprint_class(conveyor_path)
    lift_cls = rw.blueprint_class(lift_path)
    cart_cls = rw.blueprint_class(cart_path)
    generator_cls = rw.blueprint_class(f"{rw.GAMEPLAY_BP_DIR}/BP_RiftGenerator")
    if not conveyor_cls or not lift_cls or not cart_cls or not generator_cls:
        rw.log("GDD logistics pass waiting for native/Blueprint classes")
        return
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    _spawn(actors, cart_cls, "WorkshopCart", (1080, 820, 90), (0.0, 15.0, 0.0))
    generator = _spawn(actors, generator_cls, "UndergroundGenerator", (1880, -7350, -770), (0.0, 90.0, 0.0))
    if generator:
        rw.safe_set(generator, "generation_kw", 8.5)
        rw.safe_set(generator, "device_name", "Station Recovery Generator")

    conveyors = []
    for index, x in enumerate((-850, -430, -10, 410)):
        conveyor = _spawn(actors, conveyor_cls, f"StationConveyor{index:02d}", (x, -7900, -790))
        if conveyor:
            rw.safe_set(conveyor, "belt_acceleration", 640.0)
            conveyors.append(conveyor)
            _connect(generator, conveyor)

    lift = _spawn(actors, lift_cls, "StationFreightLift", (2050, -7850, -790))
    if lift:
        rw.safe_set(lift, "travel_height", 880.0)
        rw.safe_set(lift, "lift_speed", 190.0)
        _connect(generator, lift)

    _spawn(actors, cart_cls, "StationCart", (1120, -7650, -780), (0.0, -15.0, 0.0))
    level.save_current_level()
    try:
        rw.asset_library.save_directory(rw.ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    rw.log("GDD logistics ready: physics carts, powered conveyor line and functional freight lift")


if __name__ == "__main__":
    apply_all()
