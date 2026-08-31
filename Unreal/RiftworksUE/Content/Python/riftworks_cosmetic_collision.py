from __future__ import annotations

import unreal
import riftworks_setup as rw


COSMETIC_PREFIXES = (
    "RIFT_BEAUTY_",
    "RIFT_LIVED_",
)


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    components_changed = 0
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        if not label.startswith(COSMETIC_PREFIXES):
            continue

        try:
            components = actor.get_components_by_class(unreal.StaticMeshComponent)
        except Exception:
            components = []
        for component in components:
            try:
                component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                component.set_generate_overlap_events(False)
                components_changed += 1
            except Exception:
                pass

    level.save_current_level()
    rw.log(f"Cosmetic collision pass complete: {components_changed} visual mesh components set to NoCollision")


if __name__ == "__main__":
    apply_all()
