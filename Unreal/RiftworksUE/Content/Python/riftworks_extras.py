from __future__ import annotations

import unreal
import riftworks_setup as rw


def set_enum_default(bp_path: str, prop: str, enum_type_name: str, enum_value_names: list[str]) -> None:
    cdo = rw.blueprint_cdo(bp_path)
    enum_type = getattr(unreal, enum_type_name, None)
    if not cdo or not enum_type:
        return
    value = None
    for name in enum_value_names:
        value = getattr(enum_type, name, None)
        if value is not None:
            break
    if value is not None:
        rw.safe_set(cdo, prop, value)
        rw.asset_library.save_asset(bp_path, only_if_is_dirty=False)


def create_extra_blueprints() -> dict[str, str]:
    paths = {}

    rw.create_blueprint("BP_RiftHUD", rw.BP_DIR, "/Script/RiftworksUE.RiftHUD")
    hud_path = f"{rw.BP_DIR}/BP_RiftHUD"
    paths["hud"] = hud_path

    gm_path = f"{rw.BP_DIR}/BP_RiftGameMode"
    gm_cdo = rw.blueprint_cdo(gm_path)
    hud_class = rw.blueprint_class(hud_path)
    if gm_cdo and hud_class:
        rw.safe_set(gm_cdo, "hud_class", hud_class)
        rw.asset_library.save_asset(gm_path, only_if_is_dirty=False)

    joint_defs = [
        ("BP_RiftJointWeld", ["WELD"]),
        ("BP_RiftJointHinge", ["HINGE"]),
        ("BP_RiftJointSlider", ["SLIDER"]),
        ("BP_RiftJointRopeWinch", ["ROPE_WINCH", "ROPEWINCH"]),
    ]
    for name, enum_names in joint_defs:
        rw.create_blueprint(name, rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftEngineeringJoint")
        path = f"{rw.GAMEPLAY_BP_DIR}/{name}"
        set_enum_default(path, "mode", "RiftJointMode", enum_names)
        paths[name] = path

    logic_defs = [
        ("BP_RiftLogicButton", ["TOGGLE_BUTTON", "TOGGLEBUTTON"]),
        ("BP_RiftLogicSensor", ["PROXIMITY_SENSOR", "PROXIMITYSENSOR"]),
        ("BP_RiftLogicTimer", ["TIMER_PULSE", "TIMERPULSE"]),
    ]
    for name, enum_names in logic_defs:
        rw.create_blueprint(name, rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftLogicNode")
        path = f"{rw.GAMEPLAY_BP_DIR}/{name}"
        set_enum_default(path, "mode", "RiftLogicMode", enum_names)
        paths[name] = path

    rw.create_blueprint("BP_RiftBreachGolem", rw.GAMEPLAY_BP_DIR, "/Script/RiftworksUE.RiftBreachGolem")
    paths["golem"] = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachGolem"

    return paths


def apply_extra_level_setup(paths: dict[str, str]) -> None:
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    all_actors = list(actors.get_all_level_actors())
    by_label = {}
    for actor in all_actors:
        try:
            by_label[actor.get_actor_label()] = actor
        except Exception:
            pass

    # Idempotent cleanup for extras only.
    for actor in all_actors:
        try:
            if actor.get_actor_label().startswith("RIFT_EXTRA_"):
                actors.destroy_actor(actor)
        except Exception:
            pass

    floodlight = by_label.get("RIFT_AUTO_Floodlight")
    button_class = rw.blueprint_class(paths.get("BP_RiftLogicButton", ""))
    if button_class:
        button = actors.spawn_actor_from_class(button_class, unreal.Vector(520, 3770, 80), unreal.Rotator())
        if button:
            button.set_actor_label("RIFT_EXTRA_StarterLightButton")
            if floodlight:
                try:
                    button.connect_receiver(floodlight)
                    button.set_signal(True)
                except Exception as exc:
                    rw.warn(f"Starter logic connection failed: {exc}")

    sensor_class = rw.blueprint_class(paths.get("BP_RiftLogicSensor", ""))
    harpoon = by_label.get("RIFT_AUTO_Harpoon")
    if sensor_class and harpoon:
        sensor = actors.spawn_actor_from_class(sensor_class, unreal.Vector(4550, -4350, 80), unreal.Rotator())
        if sensor:
            sensor.set_actor_label("RIFT_EXTRA_HarpoonSensor")
            rw.safe_set(sensor, "sensor_radius", 1150.0)
            try:
                sensor.connect_receiver(harpoon)
            except Exception:
                pass

    golem_class = rw.blueprint_class(paths.get("golem", ""))
    if golem_class:
        for index, pos in enumerate([
            unreal.Vector(-3600, -2800, 140),
            unreal.Vector(-4700, -7600, 140),
        ]):
            golem = actors.spawn_actor_from_class(golem_class, pos, unreal.Rotator())
            if golem:
                golem.set_actor_label(f"RIFT_EXTRA_BreachGolem_{index:02d}")

    # Breach luminance at the deep end gives the underground a readable supernatural target.
    luminance_path = f"{rw.GAMEPLAY_BP_DIR}/BP_RiftBreachLuminance"
    luminance = rw.spawn_bp(actors, luminance_path, unreal.Vector(-4000, -12500, -1760))
    if luminance:
        luminance.set_actor_label("RIFT_EXTRA_DeepLuminance")

    level.save_current_level()


def apply_all() -> None:
    paths = create_extra_blueprints()
    apply_extra_level_setup(paths)
    rw.log("Blueprint extras complete: HUD, universal joints, logic nodes and Breach golems.")


if __name__ == "__main__":
    apply_all()
