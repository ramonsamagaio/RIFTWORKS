from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv

PREFIX = "RIFT_GDD_SALVAGE_"


def _set_material(component, material):
    if component and material:
        try:
            component.set_material(0, material)
        except Exception:
            pass


def _ensure_blueprints(materials):
    tiered_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftTieredSalvage"
    winch_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftRecoveryWinch"
    fabricator_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftFabricator"

    rw.create_blueprint("BP_RiftTieredSalvage", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftCapacitySalvageActor")
    rw.create_blueprint("BP_RiftRecoveryWinch", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftRecoveryWinch")
    rw.create_blueprint("BP_RiftFabricator", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftFabricator")

    winch = rw.blueprint_cdo(winch_path)
    if winch:
        try:
            _set_material(winch.get_editor_property("frame"), materials.get("metal"))
            _set_material(winch.get_editor_property("spool"), materials.get("assembly_motor"))
        except Exception:
            pass
        rw.asset_library.save_asset(winch_path, only_if_is_dirty=False)

    fabricator = rw.blueprint_cdo(fabricator_path)
    if fabricator:
        try:
            _set_material(fabricator.get_editor_property("frame"), materials.get("metal"))
            _set_material(fabricator.get_editor_property("work_surface"), materials.get("assembly"))
        except Exception:
            pass
        rw.asset_library.save_asset(fabricator_path, only_if_is_dirty=False)

    return tiered_path, winch_path, fabricator_path


def _spawn(actors, cls, label, location, rotation=None):
    actor = actors.spawn_actor_from_class(
        cls,
        unreal.Vector(*location),
        unreal.Rotator(*(rotation or (0.0, 0.0, 0.0))),
    )
    if actor:
        try:
            actor.set_actor_label(PREFIX + label)
        except Exception:
            pass
    return actor


def _spawn_tiered(
    actors,
    cls,
    label,
    location,
    item_id,
    display_name,
    tier,
    *,
    amount=1,
    heavy=False,
    mass=2.0,
    dismantle_scrap=2,
    dismantle_component=None,
    dismantle_amount=0,
    dismantleable=True,
):
    actor = _spawn(actors, cls, label, location)
    if not actor:
        return None
    rw.safe_set(actor, "item_id", item_id)
    rw.safe_set(actor, "display_name", display_name)
    rw.safe_set(actor, "amount", amount)
    rw.safe_set(actor, "b_heavy", heavy)
    rw.safe_set(actor, "mass_kg", mass)
    rw.safe_set(actor, "recovery_tier", tier)
    rw.safe_set(actor, "b_dismantleable", dismantleable)
    rw.safe_set(actor, "dismantle_scrap_yield", dismantle_scrap)
    rw.safe_set(actor, "dismantle_component_id", dismantle_component or "None")
    rw.safe_set(actor, "dismantle_component_amount", dismantle_amount)
    rw.safe_set(actor, "b_mechanically_recovered", False)
    rw.safe_set(actor, "persistent_id", f"gdd_salvage_{label.lower()}")
    try:
        actor.set_carried_state(False)
    except Exception:
        pass
    return actor


def _spawn_tool(actors, cls, label, location, item_id, display_name, required_tier):
    return _spawn_tiered(
        actors,
        cls,
        label,
        location,
        item_id,
        display_name,
        required_tier,
        heavy=False,
        mass=1.5,
        dismantle_scrap=0,
        dismantleable=False,
    )


def _stage_salvage_progression(actors, tiered_cls):
    _spawn_tool(
        actors, tiered_cls, "ToolTier2", (1325, 1040, 112),
        "recovery_tool_t2", "Pry + Cutting Recovery Kit", 1,
    )

    _spawn_tiered(
        actors, tiered_cls, "AlternatorT2", (1940, 1050, 95),
        "alternator", "Vehicle Alternator", 2,
        heavy=True, mass=19.0, dismantle_scrap=4,
        dismantle_component="copper_coil", dismantle_amount=1,
    )
    _spawn_tiered(
        actors, tiered_cls, "CompressorT2", (2220, 1190, 108),
        "compressor", "Workshop Compressor", 2,
        heavy=True, mass=34.0, dismantle_scrap=6,
        dismantle_component="motor", dismantle_amount=1,
    )

    _spawn_tool(
        actors, tiered_cls, "ToolTier3", (-300, -7440, -780),
        "recovery_tool_t3", "Powered Recovery Toolset", 2,
    )
    _spawn_tiered(
        actors, tiered_cls, "LiftMotorT3", (1160, -7870, -790),
        "industrial_motor", "Metro Lift Drive", 3,
        heavy=True, mass=92.0, dismantle_scrap=14,
        dismantle_component="motor", dismantle_amount=2,
    )
    _spawn_tiered(
        actors, tiered_cls, "PumpAssemblyT3", (-1120, -8030, -790),
        "industrial_pump", "Flood-Control Pump Assembly", 3,
        heavy=True, mass=118.0, dismantle_scrap=16,
        dismantle_component="copper_coil", dismantle_amount=2,
    )

    _spawn_tool(
        actors, tiered_cls, "ToolTier4", (360, -9870, -1170),
        "recovery_tool_t4", "Industrial Rigging + Cutting Kit", 3,
    )
    _spawn_tiered(
        actors, tiered_cls, "TransformerT4", (1510, -10020, -1160),
        "transformer_core", "Breach-Fused Transformer Core", 4,
        heavy=True, mass=245.0, dismantle_scrap=28,
        dismantle_component="breach_core", dismantle_amount=1,
    )
    _spawn_tiered(
        actors, tiered_cls, "AncientDriveT4", (-1650, -10600, -1190),
        "breach_drive", "Ancient Breach Drive", 4,
        heavy=True, mass=310.0, dismantle_scrap=34,
        dismantle_component="breach_core", dismantle_amount=2,
    )

    _spawn_tiered(
        actors, tiered_cls, "MedicalSupplies", (-1220, 1430, 112),
        "medical_supplies", "Sealed Medical Supplies", 1,
        amount=2, heavy=False, mass=0.8, dismantle_scrap=1,
        dismantleable=False,
    )

    # Extra loose loot intentionally tests the backpack limits instead of allowing infinite abstract hoarding.
    for index, x in enumerate((-1140, -1010, -880, -750, -620)):
        _spawn_tiered(
            actors, tiered_cls, f"CableBundle{index:02d}", (x, 1660, 108),
            "cable", "Salvaged Cable Bundle", 1,
            amount=3, heavy=False, mass=2.7, dismantle_scrap=1,
            dismantleable=True,
        )


def _stage_recovery_and_fabrication(actors, winch_cls, fabricator_cls):
    winch = _spawn(actors, winch_cls, "WinchTier3", (720, -7480, -770), (0.0, -80.0, 0.0))
    if winch:
        rw.safe_set(winch, "recovery_capacity_tier", 3)
        rw.safe_set(winch, "target_range", 3200.0)
        rw.safe_set(winch, "pull_force", 520000.0)

    crane = _spawn(actors, winch_cls, "RecoveryRigTier4", (820, -10180, -1160), (0.0, 20.0, 0.0))
    if crane:
        rw.safe_set(crane, "recovery_capacity_tier", 4)
        rw.safe_set(crane, "target_range", 4200.0)
        rw.safe_set(crane, "pull_force", 950000.0)
        rw.safe_set(crane, "dock_distance", 300.0)

    fabricator = _spawn(actors, fabricator_cls, "SafehouseFabricator", (-1490, 1180, 86), (0.0, 90.0, 0.0))
    if fabricator:
        rw.safe_set(fabricator, "selected_recipe", unreal.ERiftFabricationRecipe.CABLE if hasattr(unreal, "ERiftFabricationRecipe") else 1)


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    materials = rv.ensure_material_library()
    tiered_path, winch_path, fabricator_path = _ensure_blueprints(materials)

    tiered_cls = rw.blueprint_class(tiered_path)
    winch_cls = rw.blueprint_class(winch_path)
    fabricator_cls = rw.blueprint_class(fabricator_path)
    if not tiered_cls or not winch_cls or not fabricator_cls:
        rw.log("GDD salvage/fabrication pass waiting for native classes to compile")
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

    _stage_salvage_progression(actors, tiered_cls)
    _stage_recovery_and_fabrication(actors, winch_cls, fabricator_cls)

    level.save_current_level()
    try:
        rw.asset_library.save_directory(rw.ROOT, only_if_is_dirty=False, recursive=True)
    except Exception:
        pass
    rw.log("GDD salvage progression ready: backpack mass/volume limits, dismantling, T1-T4 recovery, winch/crane gates and fixed fabrication")


if __name__ == "__main__":
    apply_all()
