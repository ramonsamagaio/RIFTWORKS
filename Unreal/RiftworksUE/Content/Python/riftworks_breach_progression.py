from __future__ import annotations

import unreal
import riftworks_setup as rw

PREFIX = "RIFT_GDD_BREACH_"


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    gravity_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachGravity"
    gravity_cls = rw.blueprint_class(gravity_path)
    if not gravity_cls:
        rw.log("Gravity Breach pass waiting for BP_RiftBreachGravity")
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

    gravity = actors.spawn_actor_from_class(
        gravity_cls,
        unreal.Vector(1180.0, -10850.0, -1160.0),
        rw.rotator(),
    )
    if gravity:
        gravity.set_actor_label(PREFIX + "GravityEmitter")
        rw.safe_set(gravity, "radius", 1150.0)
        rw.safe_set(gravity, "force_strength", 185000.0)
        rw.safe_set(gravity, "b_enabled", True)

    level.save_current_level()
    rw.log("Functional Gravity Breach emitter staged in the deep chamber")


if __name__ == "__main__":
    apply_all()
