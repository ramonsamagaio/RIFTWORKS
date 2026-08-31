from __future__ import annotations

import math
import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_BEAUTY_"


def _rotator(*, pitch=0.0, yaw=0.0, roll=0.0):
    return rw.rotator(pitch=pitch, yaw=yaw, roll=roll)


def _tag(actor, name: str):
    if actor:
        try:
            actor.set_actor_label(PREFIX + name)
        except Exception:
            pass
    return actor


def _cube(actors, name, loc, size, mat, rot=None):
    return _tag(vs._spawn_cube(actors, name, loc, size, mat, rot or _rotator()), name)


def _cyl(actors, name, loc, diameter, height, mat, rot=None):
    return _tag(vs._spawn_cylinder(actors, name, loc, diameter, height, mat, rot or _rotator()), name)


def _cone(actors, name, loc, diameter, height, mat, rot=None):
    return _tag(vs._spawn_cone(actors, name, loc, diameter, height, mat, rot or _rotator()), name)


def _sphere(actors, name, loc, diameter, mat):
    mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    return _tag(vs._spawn_mesh(actors, name, mesh, loc, (diameter, diameter, diameter), mat, _rotator()), name)


def _point(actors, name, loc, color, intensity, radius, shadows=False):
    return _tag(vs._spawn_point_light(actors, name, loc, color, intensity, radius, shadows), name)


def _materials():
    mats = rv.ensure_material_library()
    extras = {
        "paint_teal": rv.create_surface_material("M_Paint_FadedTeal", (0.025, 0.105, 0.115), 0.58, 0.18, None, 0.16),
        "paint_olive": rv.create_surface_material("M_Paint_IndustrialOlive", (0.075, 0.105, 0.045), 0.66, 0.12, None, 0.18),
        "paint_red": rv.create_surface_material("M_Paint_FadedRed", (0.205, 0.042, 0.026), 0.62, 0.10, None, 0.20),
        "plaster": rv.create_surface_material("M_Plaster_Weathered", (0.165, 0.155, 0.125), 0.90, 0.0, None, 0.19),
        "wood": rv.create_surface_material("M_Wood_Utility", (0.115, 0.067, 0.032), 0.88, 0.0, None, 0.18),
        "copper": rv.create_surface_material("M_Copper_Oxidized", (0.12, 0.072, 0.035), 0.55, 0.72, None, 0.20),
        "window_warm": rv.create_surface_material("M_Window_Warm", (0.09, 0.045, 0.012), 0.24, 0.04, (2.7, 1.15, 0.22), 0.0),
        "window_cool": rv.create_surface_material("M_Window_Cool", (0.010, 0.040, 0.075), 0.20, 0.04, (0.18, 0.55, 1.8), 0.0),
        "sign_cyan": rv.create_surface_material("M_Sign_Cyan", (0.005, 0.055, 0.075), 0.30, 0.0, (0.12, 2.2, 3.2), 0.0),
        "sign_green": rv.create_surface_material("M_Sign_Green", (0.018, 0.070, 0.028), 0.30, 0.0, (0.18, 2.8, 0.42), 0.0),
        "tile": rv.create_surface_material("M_Tile_Grimy", (0.115, 0.125, 0.105), 0.62, 0.0, None, 0.10),
        "moss": rv.create_surface_material("M_Moss_Heavy", (0.020, 0.085, 0.040), 0.96, 0.0, None, 0.28),
        "road_patch": rv.create_surface_material("M_Asphalt_Patch", (0.012, 0.016, 0.019), 0.50, 0.0, None, 0.12),
    }
    mats.update(extras)
    return mats


def _awning(actors, name, x, y, z, width, depth, mats, yaw=0.0, tilt=0.0):
    rot = _rotator(yaw=yaw, roll=tilt)
    _cube(actors, name + "_Top", (x, y, z), (width, depth, 20), mats["paint_teal"], rot)
    for side in (-1, 1):
        px = x + side * width * 0.43 * math.cos(math.radians(yaw))
        py = y + side * width * 0.43 * math.sin(math.radians(yaw))
        _cube(actors, f"{name}_Brace_{side}", (px, py, z - 70), (20, 20, 150), mats["metal"])


def _pipe_run(actors, name, start, end, diameter, mat):
    ax, ay, az = start
    bx, by, bz = end
    dx, dy, dz = bx - ax, by - ay, bz - az
    length = math.sqrt(dx * dx + dy * dy + dz * dz)
    if length < 1.0:
        return
    mx, my, mz = (ax + bx) * 0.5, (ay + by) * 0.5, (az + bz) * 0.5
    yaw = math.degrees(math.atan2(dy, dx))
    horizontal = math.sqrt(dx * dx + dy * dy)
    pitch = -math.degrees(math.atan2(dz, max(horizontal, 0.001)))
    # Cylinder starts on local Z. Roll it onto X, then yaw/pitch the run.
    _cyl(actors, name, (mx, my, mz), diameter, length, mat, _rotator(pitch=pitch, yaw=yaw, roll=90.0))
    _sphere(actors, name + "_JointA", start, diameter * 1.16, mat)
    _sphere(actors, name + "_JointB", end, diameter * 1.16, mat)


def _window_bank(actors, name, y, z, xs, width, height, mats, warm_pattern=None):
    warm_pattern = warm_pattern or []
    for index, x in enumerate(xs):
        material = mats["window_warm"] if index in warm_pattern else mats["glass"]
        _cube(actors, f"{name}_Pane_{index}", (x, y, z), (width, 9, height), material)
        _cube(actors, f"{name}_Lintel_{index}", (x, y + 2, z + height * 0.55), (width + 28, 18, 16), mats["metal"])
        _cube(actors, f"{name}_Sill_{index}", (x, y + 2, z - height * 0.55), (width + 28, 22, 18), mats["concrete_dark"])


def _workshop(actors, mats):
    x, y = 1550.0, 950.0
    front = 150.0
    roof = 664.0

    # Strong base / cornice layers break the single-box silhouette.
    _cube(actors, "WorkshopPlinthFront", (x, front - 18, 45), (1960, 52, 90), mats["concrete_dark"])
    _cube(actors, "WorkshopCorniceFront", (x, front - 22, 610), (2020, 62, 58), mats["paint_teal"])
    _cube(actors, "WorkshopParapetFront", (x, front + 15, roof + 74), (1990, 46, 150), mats["metal"])
    for side in (-1, 1):
        _cube(actors, f"WorkshopParapetSide_{side}", (x + side * 965, y, roof + 74), (42, 1620, 150), mats["metal"])

    # Corrugated ribs / columns.
    for index, px in enumerate(range(650, 2460, 170)):
        if 1120 < px < 1980:
            continue
        _cube(actors, f"WorkshopFacadeRib_{index}", (px, front - 36, 330), (26, 28, 520), mats["rust"])

    # Open roller shutter parked above the entrance, not blocking gameplay.
    for strip in range(5):
        _cube(actors, f"WorkshopDoorRoll_{strip}", (x, front - 40, 545 + strip * 17), (790, 24, 12), mats["paint_olive"])
    for side in (-1, 1):
        _cube(actors, f"WorkshopDoorRail_{side}", (x + side * 410, front - 35, 305), (32, 42, 520), mats["metal"])

    _awning(actors, "WorkshopSideAwning", 2365, 630, 420, 610, 360, mats, yaw=90.0, tilt=-6.0)
    _cube(actors, "WorkshopSignFrame", (x, front - 58, 520), (720, 30, 128), mats["metal"])
    _cube(actors, "WorkshopSignGlow", (x, front - 75, 520), (575, 10, 62), mats["sign_cyan"])

    # Exterior utility spine.
    _pipe_run(actors, "WorkshopPipeVertical", (2380, 1580, 70), (2380, 1580, 590), 46, mats["copper"])
    _pipe_run(actors, "WorkshopPipeRoof", (2380, 1580, 590), (1960, 1580, 590), 46, mats["copper"])
    _cube(actors, "WorkshopMeterBox", (2390, 1518, 300), (120, 48, 180), mats["paint_olive"])
    for i in range(3):
        _cyl(actors, f"WorkshopRoofVent_{i}", (1060 + i * 300, 1120, roof + 120), 110, 190, mats["metal"])
        _cone(actors, f"WorkshopRoofVentCap_{i}", (1060 + i * 300, 1120, roof + 230), 145, 75, mats["rust"])

    _point(actors, "WorkshopWarmPool", (x, 420, 300), (255, 160, 80), 520, 820, True)
    _point(actors, "WorkshopSignBounce", (x, 40, 500), (45, 195, 230), 130, 420, False)


def _corner_store(actors, mats):
    x, y = -1480.0, 1250.0
    front = 730.0
    roof = 444.0

    _cube(actors, "StorePlinth", (x, front - 18, 36), (1430, 54, 72), mats["concrete_dark"])
    _cube(actors, "StoreFascia", (x, front - 30, 360), (1460, 64, 128), mats["paint_red"])
    _cube(actors, "StoreParapet", (x, y, roof + 58), (1440, 1060, 110), mats["plaster"])
    _awning(actors, "StoreAwning", x, front - 145, 290, 1120, 270, mats, tilt=-7.0)

    panes = [-1900, -1690, -1270, -1060]
    _window_bank(actors, "StoreFront", front - 38, 190, panes, 160, 220, mats, warm_pattern=[1, 2])
    for px in (-1995, -1795, -1585, -1375, -1165, -965):
        _cube(actors, f"StoreMullion_{int(px)}", (px, front - 48, 195), (18, 22, 270), mats["metal"])

    _cube(actors, "StoreSignBacking", (x, front - 69, 365), (790, 22, 92), mats["metal"])
    _cube(actors, "StoreSignEmissive", (x, front - 84, 365), (610, 8, 42), mats["sign_green"])
    _cube(actors, "StoreRoofAC", (-1050, 1370, roof + 85), (320, 240, 145), mats["metal"])
    for i in range(5):
        _cube(actors, f"StoreACSlat_{i}", (-1210 + i * 75, 1244, roof + 90), (40, 10, 70), mats["concrete_dark"])
    _pipe_run(actors, "StoreDrain", (-2130, 1680, 410), (-2130, 1680, 30), 34, mats["rust"])

    _point(actors, "StoreInteriorWarm", (x, 1000, 215), (255, 190, 118), 360, 650, True)
    _point(actors, "StoreSignPool", (x, 610, 345), (76, 255, 125), 95, 340, False)


def _motel(actors, mats):
    x, y = -1700.0, -1450.0
    front = -2160.0
    roof = 834.0

    _cube(actors, "MotelLowerPlinth", (x, front - 20, 50), (1910, 52, 100), mats["concrete_dark"])
    _cube(actors, "MotelUpperBand", (x, front - 25, 650), (1920, 56, 95), mats["paint_olive"])
    _cube(actors, "MotelRoofLip", (x, y, roof + 38), (1940, 1470, 76), mats["metal"])

    # Balcony rail system turns the facade into architecture instead of a slab.
    rail_y = front - 135
    _cube(actors, "MotelBalconyTopRail", (x, rail_y, 545), (1540, 24, 24), mats["metal"])
    _cube(actors, "MotelBalconyMidRail", (x, rail_y, 485), (1540, 16, 16), mats["rust"])
    for i in range(11):
        px = x - 740 + i * 148
        _cube(actors, f"MotelRailPost_{i}", (px, rail_y, 500), (16, 16, 125), mats["metal"])

    room_xs = [-2255, -1885, -1515, -1145]
    for i, px in enumerate(room_xs):
        _cube(actors, f"MotelDoorFrame_{i}", (px, front - 34, 150), (190, 34, 285), mats["concrete_dark"])
        _cube(actors, f"MotelDoorInset_{i}", (px, front - 53, 150), (145, 16, 225), mats["paint_teal"] if i != 2 else mats["rust"])
        _cube(actors, f"MotelWindow_{i}", (px + 145, front - 48, 230), (105, 10, 110), mats["window_warm"] if i in (0, 3) else mats["glass"])
        _cube(actors, f"MotelUpperWindow_{i}", (px + 70, front - 48, 610), (150, 10, 120), mats["window_cool"] if i == 1 else mats["glass"])
        _cube(actors, f"MotelRoomDivider_{i}", (px + 185, rail_y + 70, 465), (18, 260, 180), mats["metal"])

    _pipe_run(actors, "MotelGutterL", (-2610, -780, 780), (-2610, -780, 40), 32, mats["rust"])
    _pipe_run(actors, "MotelGutterR", (-790, -780, 780), (-790, -780, 40), 32, mats["rust"])
    for i in range(3):
        _cube(actors, f"MotelRoofUnit_{i}", (-2150 + i * 430, -1280, roof + 95), (260, 200, 140), mats["metal"])

    _point(actors, "MotelRoomWarmA", (-2255, -1920, 245), (255, 158, 90), 150, 390, False)
    _point(actors, "MotelRoomCool", (-1500, -1930, 620), (90, 135, 255), 85, 320, False)


def _substation(actors, mats):
    sx, sy = 1650.0, -1750.0
    # Transformer banks with coils, bushings and bus bars.
    for index, px in enumerate((1130.0, 1650.0, 2170.0)):
        _cube(actors, f"SubTransformerBody_{index}", (px, sy, 105), (360, 290, 210), mats["paint_olive"])
        for coil in (-1, 0, 1):
            _cyl(actors, f"SubCoil_{index}_{coil}", (px + coil * 78, sy, 250), 62, 150, mats["copper"])
            _cyl(actors, f"SubBushing_{index}_{coil}", (px + coil * 78, sy, 365), 32, 110, mats["glass"])
        _cube(actors, f"SubDangerPlate_{index}", (px, sy - 152, 130), (150, 10, 58), mats["hazard"])

    for line, z in enumerate((370.0, 430.0, 490.0)):
        _pipe_run(actors, f"SubBusBar_{line}", (1020, sy + line * 65 - 65, z), (2280, sy + line * 65 - 65, z), 26, mats["copper"])

    # Fence becomes more legible with horizontal courses and entry gate.
    for side_y in (-2470.0, -1030.0):
        for z in (70.0, 145.0, 220.0):
            _cube(actors, f"SubFenceRail_{int(side_y)}_{int(z)}", (sx, side_y, z), (1880, 14, 14), mats["metal"])
    _cube(actors, "SubGateL", (720, -1030, 120), (340, 22, 230), mats["metal"])
    _cube(actors, "SubGateR", (2580, -1030, 120), (340, 22, 230), mats["metal"])
    _cube(actors, "SubWarningLamp", (1650, -1038, 290), (35, 20, 35), mats["emergency"])
    _point(actors, "SubWarningLight", (1650, -1080, 280), (255, 36, 24), 105, 380, False)


def _street_furniture(actors, mats):
    # Drain grates / curb cuts / patched asphalt establish believable scale at eye level.
    for i, y in enumerate((-2900, -1800, -700, 420, 1560, 2680, 3740)):
        side = -1 if i % 2 == 0 else 1
        x = side * 438
        _cube(actors, f"Drain_{i}", (x, y, 13), (72, 135, 5), mats["metal"], _rotator(yaw=90.0))
        for slot in range(4):
            _cube(actors, f"DrainSlot_{i}_{slot}", (x + (slot - 1.5) * 14, y, 16), (6, 112, 3), mats["concrete_dark"], _rotator(yaw=90.0))

    patches = [(-120, 3150, 360, 250, -8), (180, 2050, 260, 190, 5), (-210, 620, 430, 210, 11), (130, -1450, 320, 230, -4), (-160, -2950, 400, 220, 9)]
    for i, (x, y, sx, sy, yaw) in enumerate(patches):
        _cube(actors, f"RoadPatch_{i}", (x, y, 10.5), (sx, sy, 3), mats["road_patch"], _rotator(yaw=yaw))
        # Crack strips are deliberately sparse, not a noisy texture replacement.
        _cube(actors, f"RoadCrackA_{i}", (x + 35, y - 12, 12.3), (sx * 0.52, 7, 2), mats["concrete_dark"], _rotator(yaw=yaw + 18))
        _cube(actors, f"RoadCrackB_{i}", (x - 40, y + 25, 12.3), (sx * 0.34, 6, 2), mats["concrete_dark"], _rotator(yaw=yaw - 23))

    # Bus shelter / road sign cluster.
    _cube(actors, "BusPad", (-780, 2860, 20), (360, 820, 20), mats["concrete"])
    for side in (-1, 1):
        _cube(actors, f"BusPost_{side}", (-860 + side * 120, 2860, 175), (24, 24, 310), mats["metal"])
    _cube(actors, "BusRoof", (-860, 2860, 335), (310, 740, 28), mats["paint_teal"])
    _cube(actors, "BusBackGlass", (-1018, 2860, 185), (12, 650, 260), mats["glass"])
    _cube(actors, "BusBench", (-900, 2860, 80), (110, 470, 28), mats["wood"])
    _cube(actors, "BusSign", (-710, 2480, 260), (22, 120, 170), mats["sign_cyan"])

    # Fire hydrants and utility cabinets add human scale.
    for i, (x, y) in enumerate(((-690, 1800), (720, -850), (-700, -2700))):
        _cyl(actors, f"HydrantBody_{i}", (x, y, 55), 54, 110, mats["paint_red"])
        _cyl(actors, f"HydrantCap_{i}", (x, y, 118), 70, 28, mats["metal"])
        _cyl(actors, f"HydrantSide_{i}", (x + 36, y, 68), 28, 55, mats["metal"], _rotator(roll=90.0))
    for i, (x, y) in enumerate(((850, 2050), (-840, 480), (900, -2350))):
        _cube(actors, f"UtilityCabinet_{i}", (x, y, 78), (95, 72, 156), mats["paint_olive"])
        _cube(actors, f"UtilityCabinetDoor_{i}", (x, y - 38, 82), (72, 7, 118), mats["metal"])
        _cube(actors, f"UtilityCabinetLamp_{i}", (x + 24, y - 43, 130), (12, 5, 12), mats["emergency"] if i == 2 else mats["sign_green"])


def _underground(actors, mats):
    hall_z = -925.0
    hall_y = -6900.0

    # Repeating frames produce depth cues and a proper tunnel rhythm.
    for bay in range(7):
        y = -5850.0 - bay * 390.0
        _cube(actors, f"UGFrameL_{bay}", (-820, y, hall_z + 320), (70, 80, 650), mats["concrete_dark"])
        _cube(actors, f"UGFrameR_{bay}", (820, y, hall_z + 320), (70, 80, 650), mats["concrete_dark"])
        _cube(actors, f"UGFrameTop_{bay}", (0, y, hall_z + 660), (1710, 80, 70), mats["metal"])
        if bay % 2 == 0:
            _cube(actors, f"UGFrameStripeL_{bay}", (-825, y - 42, hall_z + 210), (74, 8, 95), mats["hazard"])
            _cube(actors, f"UGFrameStripeR_{bay}", (825, y - 42, hall_z + 210), (74, 8, 95), mats["hazard"])

    # Parallel service pipes with brackets.
    for lane, x in enumerate((-730.0, -650.0, 650.0, 730.0)):
        material = mats["copper"] if lane in (0, 3) else mats["rust"]
        _pipe_run(actors, f"UGServicePipe_{lane}", (x, -5700, -360), (x, -8050, -360), 34 if lane % 2 == 0 else 26, material)
        for j in range(6):
            y = -5850 - j * 420
            _cube(actors, f"UGPipeBracket_{lane}_{j}", (x, y, -350), (62, 18, 60), mats["metal"])

    # Wall color band and directional markers.
    for side in (-1, 1):
        x = side * 875
        _cube(actors, f"UGWallBand_{side}", (x, hall_y, -650), (12, 2300, 72), mats["paint_teal"])
        for i, y in enumerate((-6150, -6900, -7650)):
            _cube(actors, f"UGMarker_{side}_{i}", (x - side * 8, y, -565), (10, 170, 70), mats["emergency"] if i == 2 else mats["sign_cyan"])

    # Ventilation fan cluster.
    for i, y in enumerate((-6220, -7440)):
        _cyl(actors, f"UGFanHousing_{i}", (865, y, -480), 250, 80, mats["metal"], _rotator(roll=90.0))
        for blade in range(4):
            _cube(actors, f"UGFanBlade_{i}_{blade}", (822, y, -480), (14, 150, 34), mats["paint_olive"], _rotator(roll=90.0, yaw=blade * 45.0))

    # Maintenance nook / clutter with practical light.
    _cube(actors, "UGToolCabinet", (-650, -7040, -820), (170, 90, 210), mats["paint_red"])
    for drawer in range(4):
        _cube(actors, f"UGToolDrawer_{drawer}", (-650, -7088, -880 + drawer * 48), (130, 8, 35), mats["metal"])
    _cube(actors, "UGWorkLamp", (-580, -7045, -610), (34, 24, 26), mats["lamp"])
    _point(actors, "UGWorkLight", (-560, -7000, -650), (255, 178, 92), 260, 520, True)

    # Deep chamber gets an authored human intervention layer around the alien geometry.
    cz = -1340.0
    cy = -9900.0
    _cube(actors, "BreachCatwalk", (0, cy + 420, cz + 95), (1550, 310, 34), mats["metal"])
    for side in (-1, 1):
        _cube(actors, f"BreachCatwalkRail_{side}", (side * 760, cy + 420, cz + 175), (20, 310, 170), mats["rust"])
    for i in range(5):
        _cube(actors, f"BreachCatwalkPost_{i}", (-650 + i * 325, cy + 560, cz + 180), (18, 18, 170), mats["metal"])
    _cube(actors, "BreachInstrumentRack", (-920, cy + 560, cz + 150), (230, 120, 260), mats["paint_olive"])
    for i in range(4):
        _cube(actors, f"BreachInstrumentLED_{i}", (-1040, cy + 500 + i * 28, cz + 180), (8, 16, 16), mats["sign_green"] if i != 2 else mats["emergency"])

    # Additional irregular low-poly crystal family to enrich silhouette.
    crystals = [
        (-1080, -10480, 270, 130, -12, 18), (-880, -10820, 190, 95, 9, 33),
        (980, -10560, 320, 150, 14, -20), (1120, -9860, 210, 100, -8, 42),
        (260, -10920, 145, 78, 20, 8),
    ]
    for i, (x, y, h, d, pitch, yaw) in enumerate(crystals):
        _cone(actors, f"BeautyCrystal_{i}", (x, y, cz + h * 0.5), d, h, mats["breach"], _rotator(pitch=pitch, yaw=yaw))

    _point(actors, "BreachResearchCool", (-880, -9520, -1120), (70, 120, 255), 150, 480, False)


def _skyline(actors, mats):
    # Cheap distant silhouettes. They are intentionally outside the playable block,
    # so collision cost is irrelevant while the horizon stops feeling empty.
    blocks = [
        (-6900, 3000, 1350, 900, 1500), (-7900, 500, 1800, 1050, 1100),
        (7200, 2800, 2100, 1200, 1650), (8200, -1200, 1600, 950, 1250),
        (-6500, -3500, 1250, 860, 980), (6800, -4300, 1750, 1000, 1400),
    ]
    for i, (x, y, w, d, h) in enumerate(blocks):
        material = mats["concrete_dark"] if i % 2 else mats["plaster"]
        _cube(actors, f"SkylineBody_{i}", (x, y, h * 0.5), (w, d, h), material)
        _cube(actors, f"SkylineCrown_{i}", (x, y, h + 80), (w * 0.72, d * 0.74, 160), mats["metal"])
        for floor in range(3):
            z = 250 + floor * max(180, h / 4.0)
            for window in (-1, 1):
                wx = x + window * w * 0.30
                face_y = y - d * 0.505
                mat = mats["window_warm"] if (i + floor + window) % 5 == 0 else mats["glass"]
                _cube(actors, f"SkylineWindow_{i}_{floor}_{window}", (wx, face_y, z), (w * 0.18, 8, 85), mat)
        if i in (1, 2, 5):
            _cyl(actors, f"SkylineAntenna_{i}", (x, y, h + 350), 22, 540, mats["metal"])
            _cube(actors, f"SkylineBeacon_{i}", (x, y, h + 625), (28, 28, 28), mats["emergency"])


def _post_process(actors):
    try:
        volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), _rotator())
        _tag(volume, "PostProcess")
        rw.safe_set(volume, "unbound", True)
        settings = volume.get_editor_property("settings")
        # Fail-soft assignments because Epic periodically renames individual settings.
        for override_name, prop_name, value in (
            ("override_vignette_intensity", "vignette_intensity", 0.22),
            ("override_bloom_intensity", "bloom_intensity", 0.20),
            ("override_motion_blur_amount", "motion_blur_amount", 0.05),
            ("override_auto_exposure_bias", "auto_exposure_bias", -0.35),
            ("override_film_slope", "film_slope", 0.88),
            ("override_film_toe", "film_toe", 0.50),
            ("override_film_shoulder", "film_shoulder", 0.22),
        ):
            rw.safe_set(settings, override_name, True)
            rw.safe_set(settings, prop_name, value)
        rw.safe_set(volume, "settings", settings)
    except Exception as exc:
        rw.warn(f"Beauty post process skipped: {exc}")


def apply_all():
    if not rw.asset_library.does_asset_exist(rw.BOOTSTRAP_MAP):
        return

    mats = _materials()
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level.load_level(rw.BOOTSTRAP_MAP)

    for actor in list(actors.get_all_level_actors()):
        try:
            if actor.get_actor_label().startswith(PREFIX):
                actors.destroy_actor(actor)
        except Exception:
            pass

    _workshop(actors, mats)
    _corner_store(actors, mats)
    _motel(actors, mats)
    _substation(actors, mats)
    _street_furniture(actors, mats)
    _underground(actors, mats)
    _skyline(actors, mats)
    _post_process(actors)

    level.save_current_level()
    rw.log(
        "Beauty pass complete: layered architecture, practical lighting, street furniture, "
        "technical underground dressing, richer Breach chamber and distant skyline"
    )


if __name__ == "__main__":
    apply_all()
