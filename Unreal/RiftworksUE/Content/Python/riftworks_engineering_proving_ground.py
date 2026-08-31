from __future__ import annotations

import unreal
import riftworks_setup as rw

PREFIX = "RIFT_GDD_MACHINE_"


def _rotator(pitch=0.0, yaw=0.0, roll=0.0):
    return rw.rotator(pitch=pitch, yaw=yaw, roll=roll)


def _spawn(actors, bp_path, label, location, rotation=(0.0, 0.0, 0.0)):
    cls = rw.blueprint_class(bp_path)
    if not cls:
        return None
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


def _physics(actor, simulate=True):
    if not actor:
        return
    try:
        mesh = actor.get_editor_property("physics_mesh")
        mesh.set_simulate_physics(simulate)
        mesh.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    except Exception:
        pass


def _joint(actors, bp_path, label, actor_a, actor_b, location):
    joint = _spawn(actors, bp_path, label, location)
    if joint and actor_a and actor_b:
        try:
            joint.attach_actors(actor_a, actor_b)
        except Exception as exc:
            rw.warn(f"Proving-ground joint failed: {exc}")
    return joint


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
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

    platform_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASPlatform"
    wheel_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASWheel"
    motor_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFASMotorWheel"
    hinge_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftJointHinge"
    button_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftLogicButton"

    origin = unreal.Vector(2650.0, 1250.0, 145.0)
    chassis = _spawn(actors, platform_path, "Cart_Chassis", (origin.x, origin.y, origin.z))
    _physics(chassis, True)

    # Wheel mesh orientation is owned by ARiftAssemblyPart::OnConstruction.
    # Keep the Actor transform neutral so the native local-Y axle is not rotated
    # a second time and remains aligned with ActorRightVector motor torque.
    wheel_specs = [
        ("FrontL", wheel_path, (origin.x + 110.0, origin.y - 145.0, origin.z - 55.0)),
        ("FrontR", wheel_path, (origin.x + 110.0, origin.y + 145.0, origin.z - 55.0)),
        ("RearL", motor_path, (origin.x - 110.0, origin.y - 145.0, origin.z - 55.0)),
        ("RearR", motor_path, (origin.x - 110.0, origin.y + 145.0, origin.z - 55.0)),
    ]
    wheels = []
    for name, path, pos in wheel_specs:
        wheel = _spawn(actors, path, f"Cart_{name}", pos)
        _physics(wheel, True)
        wheels.append(wheel)
        _joint(actors, hinge_path, f"Cart_Hinge_{name}", chassis, wheel, pos)

    button = _spawn(actors, button_path, "Cart_Control", (origin.x, origin.y, origin.z + 95.0))
    if button:
        try:
            button.connect_receiver(wheels[2])
            button.connect_receiver(wheels[3])
            button.set_signal(False)
        except Exception:
            pass

    for index, x in enumerate((3050.0, 3250.0, 3450.0)):
        part = _spawn(actors, platform_path if index == 0 else wheel_path, f"LoosePart_{index:02d}", (x, 1480.0, 120.0))
        _physics(part, False)

    level.save_current_level()
    rw.log("Engineering proving ground staged: native local-Y wheel axles + generic motor cart + loose FAS parts")


if __name__ == "__main__":
    apply_all()
