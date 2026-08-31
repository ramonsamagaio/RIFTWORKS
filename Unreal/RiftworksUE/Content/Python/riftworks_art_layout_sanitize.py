from __future__ import annotations

import unreal
import riftworks_setup as rw


SAFEHOUSE_Y_OFFSETS = {
    "RIFT_LIVED_SafehouseWorkbench": 1650.0,
    "RIFT_LIVED_SafehousePegboard": 1710.0,
}


def _set_y(actor, y):
    try:
        loc = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(loc.x, y, loc.z), False, False)
    except Exception:
        pass


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    moved = 0
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue

        if label in SAFEHOUSE_Y_OFFSETS:
            _set_y(actor, SAFEHOUSE_Y_OFFSETS[label])
            moved += 1
            continue

        if label.startswith("RIFT_LIVED_SafehouseTool_"):
            _set_y(actor, 1698.0)
            moved += 1
        elif label.startswith("RIFT_LIVED_SafehouseShelf_") or label.startswith("RIFT_LIVED_SafehouseShelfItem_") or label.startswith("RIFT_LIVED_SafehouseShelfPost_"):
            _set_y(actor, 1640.0)
            moved += 1

    level.save_current_level()
    rw.log(f"Art layout sanitize complete: {moved} safehouse art actors moved clear of gameplay devices")


if __name__ == "__main__":
    apply_all()
