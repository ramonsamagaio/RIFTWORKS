from __future__ import annotations

import unreal
import riftworks_setup as rw

PREFIX = "RIFT_GDD_CREATURE_"


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    crawler_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftCrawler"
    rw.create_blueprint("BP_RiftCrawler", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftCrawlerCreature")
    crawler_cls = rw.blueprint_class(crawler_path)
    if not crawler_cls:
        rw.log("Crawler pass waiting for RiftCrawlerCreature native class")
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

    encounters = [
        ((-1180, -7260, -790), 0.92, 215.0),
        ((1320, -8420, -890), 1.08, 245.0),
        ((-1020, -9650, -1130), 1.18, 275.0),
    ]
    for index, (location, scale, speed) in enumerate(encounters):
        crawler = actors.spawn_actor_from_class(crawler_cls, unreal.Vector(*location), rw.rotator())
        if not crawler:
            continue
        crawler.set_actor_label(f"{PREFIX}{index:02d}")
        crawler.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        rw.safe_set(crawler, "move_speed", speed)
        rw.safe_set(crawler, "health", 34.0 + index * 12.0)
        rw.safe_set(crawler, "detection_range", 1500.0 + index * 250.0)

    level.save_current_level()
    rw.log("Procedural crawler family staged in three underground depth bands")


if __name__ == "__main__":
    apply_all()
