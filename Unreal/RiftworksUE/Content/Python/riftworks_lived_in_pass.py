from __future__ import annotations

import unreal
import riftworks_setup as rw
import riftworks_visuals as rv
import riftworks_vertical_slice as vs

PREFIX = "RIFT_LIVED_"


def _rotator(*, pitch=0.0, yaw=0.0, roll=0.0):
    return rw.rotator(pitch=pitch, yaw=yaw, roll=roll)


def _tag(actor, name):
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


def _point(actors, name, loc, color, intensity, radius, shadows=False):
    return _tag(vs._spawn_point_light(actors, name, loc, color, intensity, radius, shadows), name)


def _materials():
    mats = rv.ensure_material_library()
    mats.update({
        "cloth_blue": rv.create_surface_material("M_Cloth_TarpBlue", (0.022, 0.070, 0.105), 0.92, 0.0, None, 0.14),
        "cloth_tan": rv.create_surface_material("M_Cloth_CanvasTan", (0.19, 0.145, 0.078), 0.95, 0.0, None, 0.16),
        "cardboard": rv.create_surface_material("M_Cardboard_Damp", (0.15, 0.095, 0.045), 0.96, 0.0, None, 0.20),
        "tool_dark": rv.create_surface_material("M_Tool_BlackSteel", (0.025, 0.030, 0.032), 0.52, 0.64, None, 0.06),
        "foam": rv.create_surface_material("M_Foam_Bedding", (0.135, 0.125, 0.095), 0.98, 0.0, None, 0.10),
    })
    return mats


def _safehouse(actors, mats):
    # The starting base lives inside the corner store. Make it read as an occupied
    # refuge, not as a beacon actor sitting in an empty grey shell.
    _cube(actors, "SafehouseWorkbench", (-1510, 1450, 72), (520, 110, 110), mats["wood"] if "wood" in mats else mats["trunk"])
    _cube(actors, "SafehousePegboard", (-1510, 1506, 215), (520, 16, 240), mats["paint_olive"] if "paint_olive" in mats else mats["rust"])
    for i in range(7):
        x = -1710 + i * 66
        _cube(actors, f"SafehouseTool_{i}", (x, 1494, 220 + (i % 2) * 45), (18 + (i % 3) * 8, 12, 80 + (i % 4) * 18), mats["tool_dark"], _rotator(roll=(i % 3 - 1) * 7.0))

    # Shelves with deliberately uneven supplies.
    for shelf in range(3):
        z = 70 + shelf * 95
        _cube(actors, f"SafehouseShelf_{shelf}", (-1900, 1430, z), (330, 70, 18), mats["metal"])
        for item in range(3):
            _cube(actors, f"SafehouseShelfItem_{shelf}_{item}", (-2010 + item * 105, 1430, z + 30), (55 + item * 7, 48, 44 + shelf * 7), mats["cardboard"] if item != 1 else mats["salvage"])
    for side in (-1, 1):
        _cube(actors, f"SafehouseShelfPost_{side}", (-1900 + side * 155, 1430, 165), (18, 18, 300), mats["metal"])

    # Cot / personal corner.
    _cube(actors, "SafehouseCotFrame", (-1120, 1510, 48), (360, 165, 28), mats["metal"])
    _cube(actors, "SafehouseCotMattress", (-1120, 1510, 72), (340, 150, 32), mats["foam"])
    _cube(actors, "SafehouseBlanket", (-1160, 1510, 91), (250, 155, 10), mats["cloth_blue"], _rotator(yaw=3.0))
    _cube(actors, "SafehouseCrateSeat", (-980, 1320, 52), (95, 95, 104), mats["cardboard"])

    # Battery / comms wall.
    for i in range(3):
        _cube(actors, f"SafehouseBattery_{i}", (-2090, 1130 + i * 110, 70), (110, 85, 140), mats["metal"])
        _cube(actors, f"SafehouseBatteryLED_{i}", (-2032, 1130 + i * 110, 98), (6, 24, 12), mats["lamp"] if i < 2 else mats["emergency"])
    _cube(actors, "SafehouseCommsBox", (-2050, 1030, 185), (180, 80, 160), mats["paint_teal"] if "paint_teal" in mats else mats["metal"])
    _cyl(actors, "SafehouseAntenna", (-2050, 1030, 330), 8, 210, mats["metal"])

    # Cable spool and extension lines.
    _cyl(actors, "SafehouseCableSpool", (-1040, 1170, 72), 150, 75, mats["rust"], _rotator(roll=90.0))
    for i in range(4):
        _cube(actors, f"SafehouseCableLayer_{i}", (-1040, 1136 + i * 17, 72), (130, 7, 130), mats["rubber"])

    # Tarp partition, intentionally off-axis so the interior stops feeling orthogonal.
    _cube(actors, "SafehouseTarp", (-870, 1540, 190), (18, 520, 360), mats["cloth_blue"], _rotator(yaw=-8.0))
    _cube(actors, "SafehouseTarpTop", (-870, 1540, 380), (28, 540, 22), mats["metal"], _rotator(yaw=-8.0))

    # Warm practicals. Low intensities keep flashlight important.
    for i, x in enumerate((-1820, -1450, -1100)):
        _cube(actors, f"SafehouseCeilingLamp_{i}", (x, 1240, 375), (180, 28, 18), mats["lamp"])
        _point(actors, f"SafehouseCeilingLight_{i}", (x, 1240, 335), (255, 177, 92), 210 if i == 1 else 145, 430, False)


def _workshop_interior(actors, mats):
    # Rear storage racks.
    for rack in range(3):
        x = 1050 + rack * 470
        for shelf in range(3):
            z = 85 + shelf * 140
            _cube(actors, f"WorkshopRackShelf_{rack}_{shelf}", (x, 1580, z), (360, 90, 20), mats["metal"])
        for side in (-1, 1):
            _cube(actors, f"WorkshopRackPost_{rack}_{side}", (x + side * 170, 1580, 210), (20, 20, 420), mats["rust"])
        _cube(actors, f"WorkshopEngineBlock_{rack}", (x, 1570, 120), (160, 105, 150), mats["assembly_motor"])
        _cyl(actors, f"WorkshopCan_{rack}", (x + 105, 1570, 250), 46, 92, mats["paint_red"] if "paint_red" in mats else mats["rust"])

    # Tire stacks give unmistakable automotive/workshop scale.
    for stack, x in enumerate((920, 2150)):
        for level in range(4):
            _cyl(actors, f"WorkshopTire_{stack}_{level}", (x, 650, 42 + level * 62), 120, 48, mats["rubber"])

    # Welding / fabrication table.
    _cube(actors, "WorkshopFabTable", (1580, 1040, 95), (520, 230, 35), mats["assembly"])
    for side in (-1, 1):
        _cube(actors, f"WorkshopFabLeg_{side}", (1580 + side * 220, 1040, 50), (30, 180, 100), mats["metal"])
    _cube(actors, "WorkshopVice", (1740, 1040, 145), (95, 75, 80), mats["assembly_motor"])
    _cube(actors, "WorkshopWeldingScreen", (1300, 1120, 240), (20, 360, 350), mats["cloth_tan"], _rotator(yaw=12.0))

    # Hanging lights form a visual rhythm through the garage.
    for i, x in enumerate((950, 1350, 1750, 2150)):
        _cube(actors, f"WorkshopHangCable_{i}", (x, 800, 525), (8, 8, 190), mats["rubber"])
        _cube(actors, f"WorkshopHangLamp_{i}", (x, 800, 430), (95, 35, 24), mats["lamp"] if i != 2 else mats["metal"])
        if i != 2:
            _point(actors, f"WorkshopHangLight_{i}", (x, 800, 395), (255, 184, 104), 170, 390, False)


def _street_story(actors, mats):
    # Traffic cones and abandoned maintenance gear.
    for i, (x, y, yaw) in enumerate(((-330, -2050, 4), (-210, -2180, -8), (280, 1620, 12), (340, 1770, -5))):
        _cone(actors, f"TrafficCone_{i}", (x, y, 42), 58, 84, mats["hazard"], _rotator(yaw=yaw))
        _cube(actors, f"TrafficConeBase_{i}", (x, y, 4), (76, 76, 8), mats["rubber"], _rotator(yaw=yaw))

    # Damp cardboard and trash clusters near curbs/buildings.
    clusters = [(-760, 1280), (760, 340), (-760, -1650), (720, -2950), (2440, 1260)]
    for ci, (x, y) in enumerate(clusters):
        for i in range(4):
            ox = ((i * 53 + ci * 31) % 120) - 60
            oy = ((i * 41 + ci * 17) % 110) - 55
            _cube(actors, f"TrashCard_{ci}_{i}", (x + ox, y + oy, 10 + i * 3), (70 + i * 13, 48 + (i % 2) * 20, 8), mats["cardboard"], _rotator(yaw=(ci * 19 + i * 27) % 70 - 35))
        _cyl(actors, f"TrashCan_{ci}", (x + 85, y - 40, 48), 62, 96, mats["metal"])

    # Broken road sign / improvised barricade.
    _cube(actors, "FallenSignPole", (640, -3280, 42), (22, 22, 360), mats["rust"], _rotator(pitch=72.0, yaw=18.0))
    _cube(actors, "FallenSignPanel", (760, -3240, 55), (210, 22, 135), mats["paint_teal"] if "paint_teal" in mats else mats["metal"], _rotator(pitch=72.0, yaw=18.0))
    _cube(actors, "RoadBarricadeBeam", (-580, -3600, 88), (620, 52, 42), mats["hazard"], _rotator(yaw=-12.0))
    for side in (-1, 1):
        _cube(actors, f"RoadBarricadeLeg_{side}", (-580 + side * 220, -3600, 42), (40, 130, 84), mats["metal"], _rotator(yaw=-12.0))


def _vegetation(actors, mats):
    # Vegetation is clustered around edges and cracks, never carpeted everywhere.
    shrub_points = [
        (-900, 3500), (-1000, 3200), (920, 3000), (850, 2450), (-2800, 1500),
        (-2750, -800), (2850, -2200), (2750, 950), (-2450, -3150), (2500, -3500),
    ]
    for i, (x, y) in enumerate(shrub_points):
        scale = 0.75 + (i % 4) * 0.12
        _cyl(actors, f"ShrubStem_{i}", (x, y, 45 * scale), 24 * scale, 90 * scale, mats["trunk"])
        for lobe in range(3):
            ox = (lobe - 1) * 34 * scale
            _cone(actors, f"ShrubLobe_{i}_{lobe}", (x + ox, y + (lobe % 2) * 18, 105 * scale), 120 * scale, 150 * scale, mats["foliage"], _rotator(yaw=lobe * 37.0))

    # Tufts sprouting through road/sidewalk seams.
    for i in range(18):
        side = -1 if i % 2 == 0 else 1
        x = side * (450 + (i % 3) * 40)
        y = -3300 + i * 390
        for blade in range(3):
            _cone(actors, f"Weed_{i}_{blade}", (x + blade * 8 - 8, y + blade * 11, 26), 18, 52 + blade * 11, mats["moss"] if "moss" in mats else mats["foliage"], _rotator(pitch=(blade - 1) * 7.0, yaw=blade * 31.0))

    # Moss strips on damp building edges.
    moss_strips = [
        ((590, 1460, 22), (22, 650, 38)), ((2510, 1080, 20), (20, 480, 34)),
        ((-2170, 1510, 20), (20, 370, 32)), ((-2620, -1400, 22), (22, 520, 38)),
    ]
    for i, (loc, size) in enumerate(moss_strips):
        _cube(actors, f"MossStrip_{i}", loc, size, mats["moss"] if "moss" in mats else mats["foliage"])


def _underground_guidance(actors, mats):
    # A restrained sequence of markers helps spatial memory without turning the
    # route into a glowing videogame breadcrumb trail.
    markers = [
        ((-370, -4320, 230), "emergency", (255, 44, 24)),
        ((-370, -5500, -250), "lamp", (255, 165, 85)),
        ((780, -6880, -480), "sign_cyan", (55, 115, 225)),
        ((-780, -8050, -620), "emergency", (255, 44, 24)),
        ((360, -9300, -1050), "breach", (115, 65, 255)),
    ]
    for i, (loc, key, color) in enumerate(markers):
        material = mats.get(key) or mats["lamp"]
        _cube(actors, f"RouteMarker_{i}", loc, (26, 18, 54), material)
        if i in (0, 2, 4):
            _point(actors, f"RouteMarkerLight_{i}", (loc[0], loc[1] - 25, loc[2]), color, 70 if i < 4 else 105, 260, False)


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

    _safehouse(actors, mats)
    _workshop_interior(actors, mats)
    _street_story(actors, mats)
    _vegetation(actors, mats)
    _underground_guidance(actors, mats)

    level.save_current_level()
    rw.log("Lived-in pass complete: safehouse, workshop interior, street stories, vegetation and restrained route lighting")


if __name__ == "__main__":
    apply_all()
