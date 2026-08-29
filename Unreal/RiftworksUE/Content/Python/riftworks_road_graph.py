from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_GDD_ROADS_"


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    materials = rv.ensure_material_library()
    path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftRoadGraph"
    rw.create_blueprint("BP_RiftRoadGraph", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftRoadGraph")
    cdo = rw.blueprint_cdo(path)
    if cdo:
        try:
            roads = cdo.get_editor_property("road_instances")
            curbs = cdo.get_editor_property("curb_instances")
            if roads and materials.get("asphalt"):
                roads.set_material(0, materials["asphalt"])
            if curbs and materials.get("concrete"):
                curbs.set_material(0, materials["concrete"])
        except Exception:
            pass
        rw.safe_set(cdo, "seed", 731942)
        rw.safe_set(cdo, "grid_half_extent", 4)
        rw.safe_set(cdo, "cell_size", 2800.0)
        rw.safe_set(cdo, "road_width", 720.0)
        rw.safe_set(cdo, "branch_chance", 0.66)
        rw.asset_library.save_asset(path, only_if_is_dirty=False)

    cls = rw.blueprint_class(path)
    if not cls:
        rw.log("Road graph pass waiting for native class")
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

    road = actors.spawn_actor_from_class(cls, unreal.Vector(14500.0, 8500.0, 18.0), unreal.Rotator())
    if road:
        road.set_actor_label(PREFIX + "OutskirtsNetwork")
        rw.safe_set(road, "seed", 731942)
        rw.safe_set(road, "grid_half_extent", 4)
        rw.safe_set(road, "cell_size", 2800.0)
        rw.safe_set(road, "road_width", 720.0)
        rw.safe_set(road, "branch_chance", 0.66)
        try:
            road.generate_road_graph()
        except Exception:
            pass

    level.save_current_level()
    rw.log("Deterministic connected road graph staged in the outskirts for procedural-world integration")


if __name__ == "__main__":
    apply_all()
