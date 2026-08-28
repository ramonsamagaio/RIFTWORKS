from __future__ import annotations

import unreal
import riftworks_setup as rw

PREFIX = "RIFT_STAGE_"


def _actors_by_label(actors):
    result = {}
    for actor in list(actors.get_all_level_actors()):
        try:
            result[actor.get_actor_label()] = actor
        except Exception:
            pass
    return result


def _move(actor, location, rotation=None):
    if not actor:
        return
    try:
        actor.set_actor_location(unreal.Vector(*location), False, False)
        if rotation:
            actor.set_actor_rotation(unreal.Rotator(*rotation), False)
    except Exception:
        pass


def _spawn_salvage(actors, cls, name, location, item_id, display_name, amount=1, heavy=False, mass=2.0):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator())
    if not actor:
        return None
    try:
        actor.set_actor_label(PREFIX + name)
    except Exception:
        pass
    rw.safe_set(actor, "item_id", item_id)
    rw.safe_set(actor, "display_name", display_name)
    rw.safe_set(actor, "amount", amount)
    rw.safe_set(actor, "b_heavy", heavy)
    rw.safe_set(actor, "mass_kg", mass)
    rw.safe_set(actor, "persistent_id", f"vertical_slice_{name.lower()}")
    try:
        actor.set_carried_state(False)
    except Exception:
        pass
    return actor


def _stage_starting_base(by_label):
    # Corner store becomes an immediately legible safehouse instead of placing the base machinery naked in the road.
    _move(by_label.get("RIFT_AUTO_StarterBase"), (-1500, 1380, 72))
    _move(by_label.get("RIFT_AUTO_Generator"), (-2060, 1480, 62), (0, 90, 0))
    _move(by_label.get("RIFT_AUTO_Battery"), (-1880, 1300, 62), (0, 90, 0))
    _move(by_label.get("RIFT_AUTO_Floodlight"), (-930, 690, 92), (0, -20, 0))
    _move(by_label.get("RIFT_EXTRA_StarterLightButton"), (-1130, 800, 62))


def _stage_hostiles(by_label):
    # Encounters are occluded by actual architecture so the player discovers them rather than aggroing the whole map at spawn.
    positions = {
        "RIFT_AUTO_Humanoid_00": (1560, 1320, 105),       # inside workshop
        "RIFT_AUTO_Humanoid_01": (-1760, -1420, 105),    # inside motel shell
        "RIFT_AUTO_Humanoid_02": (-620, -2180, 105),     # checkpoint booth/road block
        "RIFT_AUTO_Humanoid_03": (-640, -7500, -820),    # station hall
        "RIFT_AUTO_Humanoid_04": (620, -10200, -1240),   # Breach researchers/cultist
        "RIFT_EXTRA_BreachGolem_00": (610, -7750, -800),
        "RIFT_EXTRA_BreachGolem_01": (-780, -10400, -1230),
    }
    for label, pos in positions.items():
        _move(by_label.get(label), pos)


def _stage_colossus(by_label):
    giant = by_label.get("RIFT_AUTO_WalkerColossus")
    # 60m-ish away from spawn: readable against the skyline but not instantly in melee range.
    _move(giant, (5200, -4700, 0), (0, 145, 0))
    harpoon = by_label.get("RIFT_AUTO_Harpoon")
    _move(harpoon, (3600, -3500, 70), (0, -35, 0))
    sensor = by_label.get("RIFT_EXTRA_HarpoonSensor")
    _move(sensor, (3750, -3700, 70))


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    # Rebuild staged loot deterministically.
    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    by_label = _actors_by_label(actors)
    _stage_starting_base(by_label)
    _stage_hostiles(by_label)
    _stage_colossus(by_label)

    salvage_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftSalvage"
    salvage_cls = rw.blueprint_class(salvage_path)
    if salvage_cls:
        # Workshop: mechanically useful, heavy salvage lives where the player expects it.
        _spawn_salvage(actors, salvage_cls, "WorkshopMotor", (1510, 1120, 86), "motor", "Industrial Drive Motor", 1, True, 31.0)
        _spawn_salvage(actors, salvage_cls, "WorkshopCable", (1780, 1250, 75), "cable", "Heavy Cable Coil", 3, False, 4.0)
        _spawn_salvage(actors, salvage_cls, "WorkshopElectronics", (1260, 1250, 95), "electronics", "Motor Controller", 2, False, 1.5)
        _spawn_salvage(actors, salvage_cls, "WorkshopScrap", (2050, 1350, 65), "scrap", "Machined Scrap", 6, False, 2.0)

        # Starting safehouse/store: modest supplies, enough to encourage touching the systems.
        _spawn_salvage(actors, salvage_cls, "StoreBattery", (-1700, 1460, 105), "battery", "Rechargeable Battery Pack", 2, False, 1.4)
        _spawn_salvage(actors, salvage_cls, "StoreElectronics", (-1300, 1500, 105), "electronics", "Consumer Electronics", 2, False, 0.8)

        # Substation: power-system rewards.
        _spawn_salvage(actors, salvage_cls, "SubstationCoil", (1650, -1750, 310), "copper_coil", "Transformer Copper Coil", 1, True, 36.0)
        _spawn_salvage(actors, salvage_cls, "SubstationControl", (2470, -1680, 220), "electronics", "Grid Control Module", 3, False, 2.2)

        # Underground: increasingly unusual salvage.
        _spawn_salvage(actors, salvage_cls, "UndergroundPump", (560, -7650, -815), "motor", "Submersible Pump Motor", 1, True, 44.0)
        _spawn_salvage(actors, salvage_cls, "UndergroundCable", (-510, -7700, -835), "cable", "Armored Power Cable", 4, False, 5.0)
        _spawn_salvage(actors, salvage_cls, "BreachShard", (-700, -10020, -1225), "breach_core", "Unstable Breach Core", 1, True, 24.0)

    level.save_current_level()
    rw.log("Gameplay staging complete: safehouse, occluded encounters, physical loot and visible Colossus route")


if __name__ == "__main__":
    apply_all()
