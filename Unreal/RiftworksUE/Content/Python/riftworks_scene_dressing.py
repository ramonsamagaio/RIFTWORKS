from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_DRESS_"


def _label(actor, name: str):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cube(actors, name, loc, size, mat, rot=None):
    actor = vs._spawn_cube(actors, name, loc, size, mat, rot)
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cylinder(actors, name, loc, diameter, height, mat, rot=None):
    actor = vs._spawn_cylinder(actors, name, loc, diameter, height, mat, rot)
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _point(actors, name, loc, color, intensity, radius, shadows=False):
    actor = vs._spawn_point_light(actors, name, loc, color, intensity, radius, shadows)
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _spawn_dumpster(actors, name, loc, yaw, mats):
    x, y, z = loc
    rot = unreal.Rotator(0, yaw, 0)
    _cube(actors, name + "_Bin", (x, y, z + 65), (260, 150, 130), mats["metal"], rot)
    _cube(actors, name + "_Lid", (x, y, z + 137), (270, 160, 12), mats["rust"], rot)
    for sx in (-1, 1):
        _cylinder(actors, f"{name}_Wheel_{sx}", (x + sx * 82, y, z + 15), 34, 24, mats["rubber"], unreal.Rotator(90, yaw, 0))


def _spawn_pallet_stack(actors, name, loc, mats, levels=3):
    x, y, z = loc
    for level in range(levels):
        zz = z + 16 + level * 34
        for strip in (-1, 0, 1):
            _cube(actors, f"{name}_L{level}_S{strip}", (x + strip * 35, y, zz), (30, 130, 18), mats["trunk"])
        _cube(actors, f"{name}_L{level}_CrossA", (x, y - 45, zz + 12), (135, 18, 12), mats["trunk"])
        _cube(actors, f"{name}_L{level}_CrossB", (x, y + 45, zz + 12), (135, 18, 12), mats["trunk"])


def _spawn_barrels(actors, base_name, loc, mats, count=4):
    x, y, z = loc
    for i in range(count):
        px = x + (i % 2) * 72
        py = y + (i // 2) * 72
        material = mats["hazard"] if i == 0 else (mats["rust"] if i % 2 else mats["metal"])
        _cylinder(actors, f"{base_name}_{i}", (px, py, z + 52), 58, 104, material)


def _spawn_utility_pole(actors, name, loc, mats, damaged=False):
    x, y, z = loc
    yaw = 7 if damaged else 0
    rot = unreal.Rotator(0, yaw, 0)
    _cylinder(actors, name + "_Pole", (x, y, z + 310), 38, 620, mats["trunk"], rot)
    _cube(actors, name + "_Cross", (x, y, z + 570), (310, 32, 30), mats["trunk"], rot)
    for offset in (-105, 0, 105):
        _cylinder(actors, f"{name}_Insulator_{offset}", (x + offset, y, z + 610), 20, 45, mats["glass"])


def _spawn_powerline_segment(actors, name, a, b, mats):
    ax, ay, az = a
    bx, by, bz = b
    dx, dy, dz = bx - ax, by - ay, bz - az
    length = (dx * dx + dy * dy + dz * dz) ** 0.5
    if length <= 1:
        return
    midpoint = ((ax + bx) * 0.5, (ay + by) * 0.5, (az + bz) * 0.5 - 18)
    yaw = unreal.MathLibrary.atan2(dy, dx) * 57.2957795
    pitch = -unreal.MathLibrary.atan2(dz, max(1.0, (dx * dx + dy * dy) ** 0.5)) * 57.2957795
    _cube(actors, name, midpoint, (length, 7, 7), mats["rubber"], unreal.Rotator(pitch, yaw, 0))


def _spawn_rooftop_hvac(actors, name, loc, mats):
    x, y, z = loc
    _cube(actors, name + "_Base", (x, y, z + 55), (220, 180, 110), mats["metal"])
    _cube(actors, name + "_Top", (x, y, z + 122), (240, 200, 24), mats["rust"])
    for side in (-1, 1):
        _cube(actors, f"{name}_Vent_{side}", (x + side * 80, y - 92, z + 65), (70, 16, 54), mats["concrete_dark"])


def _spawn_abandoned_checkpoint(actors, mats):
    # A small human story beat: road was once controlled and abandoned in a hurry.
    y = -2300
    for side in (-1, 1):
        _cube(actors, f"CheckpointBarrier_{side}", (side * 250, y, 58), (330, 54, 70), mats["hazard"], unreal.Rotator(0, side * 8, 0))
        _cube(actors, f"CheckpointFoot_{side}", (side * 250, y, 18), (80, 160, 36), mats["concrete"])
    _cube(actors, "CheckpointBoothFloor", (-620, y + 110, 10), (430, 390, 20), mats["concrete"])
    _cube(actors, "CheckpointBoothRear", (-620, y + 285, 145), (430, 28, 290), mats["metal"])
    _cube(actors, "CheckpointBoothLeft", (-820, y + 110, 145), (28, 390, 290), mats["metal"])
    _cube(actors, "CheckpointBoothRight", (-420, y + 110, 145), (28, 390, 290), mats["metal"])
    _cube(actors, "CheckpointBoothRoof", (-620, y + 110, 300), (470, 430, 24), mats["rust"])
    _cube(actors, "CheckpointBoothWindow", (-620, y - 92, 175), (250, 10, 115), mats["glass"])
    _cube(actors, "CheckpointRedLamp", (-750, y - 103, 255), (24, 18, 24), mats["emergency"])
    _point(actors, "CheckpointEmergency", (-750, y - 130, 250), (255, 35, 22), 180, 380, False)


def _spawn_workshop_detail(actors, mats):
    # Exterior equipment gives the workshop a readable scavenging identity.
    _spawn_dumpster(actors, "WorkshopDumpster", (2380, 1180, 0), -8, mats)
    _spawn_pallet_stack(actors, "WorkshopPallets", (2240, 520, 0), mats, 4)
    _spawn_barrels(actors, "WorkshopBarrels", (980, 1640, 0), mats, 5)
    _spawn_rooftop_hvac(actors, "WorkshopHVAC", (1550, 960, 678), mats)
    _cube(actors, "WorkshopSignBack", (1550, 125, 515), (590, 28, 120), mats["rust"])
    _cube(actors, "WorkshopSignGlow", (1550, 107, 515), (440, 10, 60), mats["lamp"])
    _point(actors, "WorkshopInteriorWarm", (1550, 900, 300), (255, 174, 92), 560, 900, True)

    # Simple lift frame, a visual tease for future mechanical construction.
    for side in (-1, 1):
        _cube(actors, f"WorkshopLiftPost_{side}", (1550 + side * 270, 1150, 170), (45, 45, 340), mats["assembly"])
        _cube(actors, f"WorkshopLiftArm_{side}", (1550 + side * 150, 1150, 110), (260, 32, 30), mats["assembly_motor"])


def _spawn_store_detail(actors, mats):
    _spawn_dumpster(actors, "StoreDumpster", (-2190, 1480, 0), 10, mats)
    for i in range(3):
        _cube(actors, f"StoreShelf_{i}", (-1650 + i * 190, 1450, 105), (130, 460, 210), mats["metal"])
    _cube(actors, "StoreCounter", (-1340, 965, 62), (560, 100, 125), mats["trunk"])
    _cube(actors, "StoreEmergencyGlow", (-1110, 735, 280), (45, 14, 30), mats["emergency"])
    _point(actors, "StoreEmergencyLight", (-1110, 755, 280), (255, 46, 30), 170, 450, False)


def _spawn_motel_detail(actors, mats):
    _spawn_pallet_stack(actors, "MotelBoardedPallets", (-2500, -1150, 0), mats, 2)
    for room in range(4):
        x = -2260 + room * 370
        _cube(actors, f"MotelDoor_{room}", (x, -2168, 110), (145, 14, 220), mats["rust"] if room == 2 else mats["metal"])
        _cube(actors, f"MotelNumber_{room}", (x, -2178, 220), (55, 6, 28), mats["road_white"])
    _cube(actors, "MotelSignPost", (-2740, -980, 190), (55, 55, 380), mats["rust"])
    _cube(actors, "MotelSign", (-2740, -980, 405), (360, 70, 150), mats["emergency"])
    _point(actors, "MotelSignLight", (-2690, -980, 390), (186, 36, 24), 260, 600, False)


def _spawn_substation_detail(actors, mats):
    sx, sy = 1650, -1750
    for row in range(2):
        for col in range(3):
            x = sx - 520 + col * 520
            y = sy - 320 + row * 620
            _cylinder(actors, f"SubInsulator_{row}_{col}", (x, y, 275), 42, 260, mats["glass"])
    # Control shack with little status lights.
    _cube(actors, "SubControlFloor", (2500, -1750, 12), (440, 520, 24), mats["concrete"])
    _cube(actors, "SubControlBody", (2500, -1750, 155), (430, 510, 285), mats["metal"])
    _cube(actors, "SubControlDoor", (2279, -1750, 130), (12, 150, 245), mats["rust"])
    for idx, (z, matkey) in enumerate(((170, "lamp"), (210, "emergency"), (250, "lamp"))):
        _cube(actors, f"SubStatus_{idx}", (2270, -1640 + idx * 65, z), (10, 22, 22), mats[matkey])


def _spawn_underground_detail(actors, mats):
    # Cable trays and pipes frame the path through the station.
    for i in range(6):
        y = -6100 - i * 360
        _cube(actors, f"UG_CableTray_{i}", (-760, y, -390), (30, 330, 24), mats["metal"])
        _cylinder(actors, f"UG_PipeRed_{i}", (745, y, -380), 36, 340, mats["rust"], unreal.Rotator(90, 0, 0))
    # Benches and debris in the station hall.
    for i, x in enumerate((-520, 0, 520)):
        _cube(actors, f"UG_BenchSeat_{i}", (x, -7050, -845), (280, 70, 24), mats["trunk"])
        _cube(actors, f"UG_BenchLegA_{i}", (x - 90, -7050, -885), (24, 54, 80), mats["metal"])
        _cube(actors, f"UG_BenchLegB_{i}", (x + 90, -7050, -885), (24, 54, 80), mats["metal"])
    _spawn_barrels(actors, "UG_Barrels", (510, -7800, -925), mats, 4)
    _spawn_pallet_stack(actors, "UG_Pallets", (-590, -7800, -925), mats, 3)

    # Breach chamber's human investigation station, implying someone got here first.
    _cube(actors, "BreachResearchTable", (-720, -9700, -1265), (420, 120, 105), mats["metal"])
    _cube(actors, "BreachResearchMonitor", (-720, -9680, -1160), (120, 30, 90), mats["breach"])
    _cube(actors, "BreachGenerator", (780, -9720, -1240), (240, 180, 180), mats["rust"])
    _cube(actors, "BreachCableA", (380, -9810, -1320), (620, 18, 18), mats["rubber"], unreal.Rotator(0, -12, 0))
    _point(actors, "BreachResearchGlow", (-700, -9700, -1110), (91, 72, 255), 330, 520, False)


def _spawn_powerline_dressing(actors, mats):
    poles = [(-2550, -3300, 0), (-2550, -1700, 0), (-2550, 0, 0), (-2550, 1750, 0), (-2550, 3400, 0)]
    tops = []
    for i, pos in enumerate(poles):
        _spawn_utility_pole(actors, f"PowerPole_{i}", pos, mats, damaged=(i == 2))
        tops.append((pos[0], pos[1], 610))
    for i in range(len(tops) - 1):
        for lane in (-105, 0, 105):
            a = (tops[i][0] + lane, tops[i][1], tops[i][2])
            b = (tops[i + 1][0] + lane, tops[i + 1][1], tops[i + 1][2])
            _spawn_powerline_segment(actors, f"PowerLine_{i}_{lane}", a, b, mats)


def _reposition_gameplay_actors(actors):
    positions = {
        "RIFT_AUTO_Humanoid_00": unreal.Vector(1180, -900, 105),
        "RIFT_AUTO_Humanoid_01": unreal.Vector(-1550, -1480, 105),
        "RIFT_AUTO_Humanoid_02": unreal.Vector(1150, 1900, 105),
        "RIFT_AUTO_Humanoid_03": unreal.Vector(-850, -7350, -820),
        "RIFT_AUTO_Humanoid_04": unreal.Vector(650, -10050, -1250),
        "RIFT_EXTRA_BreachGolem_00": unreal.Vector(700, -7700, -790),
        "RIFT_EXTRA_BreachGolem_01": unreal.Vector(-850, -10200, -1240),
    }
    for actor in list(actors.get_all_level_actors()):
        try:
            label = actor.get_actor_label()
        except Exception:
            continue
        location = positions.get(label)
        if location is not None:
            try:
                actor.set_actor_location(location, False, False)
            except Exception:
                pass


def apply_all() -> None:
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return
    mats = rv.ensure_material_library()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    _spawn_abandoned_checkpoint(actors, mats)
    _spawn_workshop_detail(actors, mats)
    _spawn_store_detail(actors, mats)
    _spawn_motel_detail(actors, mats)
    _spawn_substation_detail(actors, mats)
    _spawn_underground_detail(actors, mats)
    _spawn_powerline_dressing(actors, mats)
    _reposition_gameplay_actors(actors)

    level.save_current_level()
    rw.log("Scene dressing complete: urban props, industrial details, environmental storytelling and encounter staging")


if __name__ == "__main__":
    apply_all()
