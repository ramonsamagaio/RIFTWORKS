extends Node3D

const WORLD_SIZE := 220.0
const START_POSITION := Vector3(0, 0.1, 44)

var rng := RandomNumberGenerator.new()
var player: RiftPlayer
var grid: PowerGridSystem
var hud: Label
var prompt: Label
var objective: Label
var status_message := ""
var status_timer := 0.0

func _ready() -> void:
    rng.seed = 1337
    _make_environment()
    _make_ground()
    _make_roads()
    _make_city()
    _make_warehouse(Vector3(23, 0, 7))
    _make_start_shelter()
    _make_underground()
    _make_power_grid()
    _make_player()
    _populate_salvage()
    _spawn_humanoids()
    _spawn_colossus()
    _make_hud()
    status_message = "Find real components, establish power, then push below the city."
    status_timer = 8.0

func _process(delta: float) -> void:
    status_timer = maxf(0.0, status_timer - delta)
    if not is_instance_valid(player) or not is_instance_valid(hud):
        return
    hud.text = "RIFTWORKS  //  THE GRID IS DARK\n%s\n%s" % [player.get_hud_status(), grid.get_summary() if is_instance_valid(grid) else "GRID OFFLINE"]
    prompt.text = player.interaction_prompt
    objective.text = status_message if status_timer > 0.0 else "B floodlight   G generator   T battery   R reload flashlight   LMB strike   V shoulder"

func _make_environment() -> void:
    var world_env := WorldEnvironment.new()
    world_env.name = "NightEnvironment"
    var env := Environment.new()
    env.background_mode = Environment.BG_COLOR
    env.background_color = Color("02040a")
    env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    env.ambient_light_color = Color("111729")
    env.ambient_light_energy = 0.075
    env.reflected_light_source = Environment.REFLECTION_SOURCE_DISABLED

    env.tonemap_mode = Environment.TONE_MAPPER_AGX
    env.tonemap_exposure = 1.08
    env.tonemap_agx_contrast = 1.28
    env.glow_enabled = true
    env.glow_intensity = 0.48
    env.glow_bloom = 0.08
    env.glow_hdr_threshold = 1.15

    env.ssao_enabled = true
    env.ssao_radius = 1.45
    env.ssao_intensity = 2.4
    env.ssao_power = 1.35
    env.ssao_detail = 0.75
    env.ssil_enabled = true
    env.ssil_radius = 4.0
    env.ssil_intensity = 1.15
    env.ssil_normal_rejection = 1.0

    env.sdfgi_enabled = true
    env.sdfgi_use_occlusion = true
    env.sdfgi_cascades = 3
    env.sdfgi_min_cell_size = 0.45
    env.sdfgi_energy = 0.72
    env.sdfgi_read_sky_light = false

    env.fog_enabled = true
    env.fog_light_color = Color("0b1120")
    env.fog_light_energy = 0.38
    env.fog_density = 0.0045
    env.fog_height = 1.2
    env.fog_height_density = 0.025

    env.volumetric_fog_enabled = true
    env.volumetric_fog_density = 0.0085
    env.volumetric_fog_albedo = Color(0.68, 0.72, 0.80)
    env.volumetric_fog_emission = Color("02040a")
    env.volumetric_fog_emission_energy = 0.08
    env.volumetric_fog_length = 72.0
    env.volumetric_fog_detail_spread = 1.7
    env.volumetric_fog_anisotropy = 0.62
    env.volumetric_fog_ambient_inject = 0.04
    env.volumetric_fog_temporal_reprojection_enabled = true
    env.volumetric_fog_temporal_reprojection_amount = 0.88
    world_env.environment = env
    add_child(world_env)

    var moon := DirectionalLight3D.new()
    moon.name = "ColdMoonScatter"
    moon.light_color = Color("687cae")
    moon.light_energy = 0.13
    moon.light_indirect_energy = 0.28
    moon.light_volumetric_fog_energy = 0.22
    moon.rotation_degrees = Vector3(-54, -31, 0)
    moon.shadow_enabled = true
    moon.directional_shadow_max_distance = 120.0
    moon.directional_shadow_fade_start = 0.78
    add_child(moon)

func _mat(color: Color, metallic := 0.0, roughness := 0.86) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _box(pos: Vector3, size: Vector3, color: Color, parent: Node3D = null, collision := true, metallic := 0.0) -> Node3D:
    var target_parent := parent if parent != null else self
    var node: Node3D
    if collision:
        var body := StaticBody3D.new()
        node = body
        var collision_shape := CollisionShape3D.new()
        var shape := BoxShape3D.new()
        shape.size = size
        collision_shape.shape = shape
        body.add_child(collision_shape)
    else:
        node = Node3D.new()
    node.position = pos
    var mi := MeshInstance3D.new()
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color, metallic)
    mi.mesh = mesh
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    node.add_child(mi)
    target_parent.add_child(node)
    return node

func _cylinder(pos: Vector3, radius: float, height: float, color: Color, parent: Node3D = null, collision := false) -> Node3D:
    var target_parent := parent if parent != null else self
    var node := Node3D.new()
    node.position = pos
    var mi := MeshInstance3D.new()
    var mesh := CylinderMesh.new()
    mesh.top_radius = radius
    mesh.bottom_radius = radius
    mesh.height = height
    mesh.radial_segments = 8
    mesh.material = _mat(color, 0.45, 0.7)
    mi.mesh = mesh
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    node.add_child(mi)
    if collision:
        var body := StaticBody3D.new()
        var cs := CollisionShape3D.new()
        var shape := CylinderShape3D.new()
        shape.radius = radius
        shape.height = height
        cs.shape = shape
        body.add_child(cs)
        node.add_child(body)
    target_parent.add_child(node)
    return node

func _make_ground() -> void:
    var tile := 20.0
    for gx in range(-5, 6):
        for gz in range(-5, 6):
            if gx == -2 and gz == -1:
                continue
            var variation := rng.randf_range(-0.012, 0.012)
            _box(Vector3(gx * tile, -0.55, gz * tile), Vector3(tile + 0.15, 1.0, tile + 0.15), Color(0.055 + variation, 0.064 + variation, 0.073 + variation), null, true)

func _make_roads() -> void:
    var asphalt := Color("15191e")
    for x in [-56.0, -28.0, 0.0, 28.0, 56.0]:
        _box(Vector3(x, 0.025, 0), Vector3(8.0, 0.05, 205.0), asphalt, null, false)
    for z in [-70.0, -42.0, -14.0, 14.0, 42.0, 70.0]:
        _box(Vector3(0, 0.028, z), Vector3(150.0, 0.05, 8.0), asphalt, null, false)

func _make_city() -> void:
    for bx in range(-2, 3):
        for bz in range(-3, 4):
            var center := Vector3(bx * 28.0 + 14.0, 0, bz * 28.0)
            if center.distance_to(START_POSITION) < 22.0 or center.distance_to(Vector3(-40,0,-20)) < 20.0:
                continue
            if center.distance_to(Vector3(23,0,7)) < 17.0:
                continue
            if rng.randf() < 0.17:
                continue
            var w := rng.randf_range(11.0, 17.0)
            var d := rng.randf_range(10.0, 17.0)
            var floors := rng.randi_range(1, 4)
            var h := floors * rng.randf_range(3.1, 3.7)
            var tint := rng.randf_range(0.0, 0.035)
            var color := Color(0.075 + tint, 0.079 + tint * 0.7, 0.083 + tint * 0.5)
            _box(center + Vector3(rng.randf_range(-2,2), h * 0.5, rng.randf_range(-2,2)), Vector3(w,h,d), color)
            if rng.randf() < 0.7:
                _box(center + Vector3(rng.randf_range(-2,2), h + 0.45, rng.randf_range(-2,2)), Vector3(rng.randf_range(2,5),0.8,rng.randf_range(2,4)), Color("24292d"), null, true, 0.25)

    for z in range(-84, 85, 14):
        if abs(z + 20) < 8:
            continue
        _make_dead_streetlight(Vector3(-3.8, 0, z))
        if z % 28 == 0:
            _make_dead_streetlight(Vector3(31.8, 0, z))

func _make_dead_streetlight(pos: Vector3) -> void:
    var root := Node3D.new()
    root.position = pos
    add_child(root)
    _box(Vector3(0, 2.2, 0), Vector3(0.12, 4.4, 0.12), Color("30353a"), root, false, 0.65)
    _box(Vector3(0.42, 4.25, 0), Vector3(0.9, 0.12, 0.12), Color("30353a"), root, false, 0.65)
    _box(Vector3(0.83, 4.08, 0), Vector3(0.38, 0.22, 0.32), Color("252a2e"), root, false, 0.35)

func _make_warehouse(center: Vector3) -> void:
    var root := Node3D.new()
    root.name = "OpenWarehousePOI"
    root.position = center
    add_child(root)
    _box(Vector3(0, 0.05, 0), Vector3(18, 0.1, 15), Color("202329"), root)
    _box(Vector3(-8.75, 3.0, 0), Vector3(0.5, 6.0, 15), Color("35393d"), root)
    _box(Vector3(8.75, 3.0, 0), Vector3(0.5, 6.0, 15), Color("35393d"), root)
    _box(Vector3(0, 3.0, 7.25), Vector3(18, 6.0, 0.5), Color("35393d"), root)
    _box(Vector3(-6.25, 3.0, -7.25), Vector3(5.5, 6.0, 0.5), Color("35393d"), root)
    _box(Vector3(6.25, 3.0, -7.25), Vector3(5.5, 6.0, 0.5), Color("35393d"), root)
    _box(Vector3(0, 5.9, 0), Vector3(18.2, 0.35, 15.2), Color("292d31"), root, true, 0.42)
    for x in [-5.5, 0.0, 5.5]:
        _box(Vector3(x, 0.55, 2.0), Vector3(2.7, 1.1, 1.2), Color("2c3135"), root, true, 0.55)

func _make_start_shelter() -> void:
    var root := Node3D.new()
    root.name = "StarterGarage"
    root.position = Vector3(0, 0, 38)
    add_child(root)
    _box(Vector3(0, 0.03, 0), Vector3(10, 0.08, 9), Color("22272c"), root)
    _box(Vector3(-4.8, 2.2, 0), Vector3(0.4, 4.4, 9), Color("30363b"), root)
    _box(Vector3(4.8, 2.2, 0), Vector3(0.4, 4.4, 9), Color("30363b"), root)
    _box(Vector3(0, 2.2, 4.3), Vector3(10, 4.4, 0.4), Color("30363b"), root)
    _box(Vector3(0, 4.35, 0), Vector3(10.2, 0.3, 9.2), Color("292f33"), root)

func _make_underground() -> void:
    var entrance := Node3D.new()
    entrance.name = "UndergroundEntrance"
    entrance.position = Vector3(-40, 0, -20)
    add_child(entrance)

    for i in range(11):
        var z := i * -1.55 + 7.0
        var y := -0.45 - i * 0.72
        _box(Vector3(0, y, z), Vector3(7.0, 0.42, 1.6), Color("252a30"), entrance)
        _box(Vector3(-3.6, y + 1.7, z), Vector3(0.35, 3.8, 1.6), Color("242a2f"), entrance)
        _box(Vector3(3.6, y + 1.7, z), Vector3(0.35, 3.8, 1.6), Color("242a2f"), entrance)

    var tunnel_center := Vector3(-40, -9.2, -42)
    _box(tunnel_center + Vector3(0,-1.1,0), Vector3(8,0.5,42), Color("20252a"))
    _box(tunnel_center + Vector3(-4.1,2.0,0), Vector3(0.45,6.5,42), Color("252b31"))
    _box(tunnel_center + Vector3(4.1,2.0,0), Vector3(0.45,6.5,42), Color("252b31"))
    _box(tunnel_center + Vector3(0,5.0,0), Vector3(8.6,0.4,42), Color("1c2126"))

    for i in range(8):
        var z2 := -62.0 - i * 1.7
        var y2 := -10.0 - i * 0.9
        _box(Vector3(-40,y2,z2), Vector3(8,0.45,1.8), Color("22282e"))

    var chamber := Vector3(-40, -18.2, -89)
    _box(chamber + Vector3(0,-1.0,0), Vector3(34,0.6,34), Color("181d22"))
    _box(chamber + Vector3(-17,5,0), Vector3(0.6,12,34), Color("20252b"))
    _box(chamber + Vector3(17,5,0), Vector3(0.6,12,34), Color("20252b"))
    var breach := OmniLight3D.new()
    breach.position = chamber + Vector3(0, 2.0, -7)
    breach.light_color = Color("8067d6")
    breach.light_energy = 6.5
    breach.omni_range = 23.0
    breach.light_size = 1.4
    breach.light_volumetric_fog_energy = 2.4
    breach.shadow_enabled = true
    add_child(breach)
    _cylinder(chamber + Vector3(0,1.4,-7), 1.3, 2.8, Color("332e48"), null, false)

func _make_power_grid() -> void:
    grid = PowerGridSystem.new()
    grid.name = "PowerGrid"
    add_child(grid)
    var generator := _spawn_power_device(PowerDevice.Kind.GENERATOR, Vector3(-2.7,0.58,38), "Portable Generator", 3.2)
    var battery := _spawn_power_device(PowerDevice.Kind.BATTERY, Vector3(0.2,0.68,38), "Starter Battery Bank", 5.0, 3.8)
    var light := _spawn_power_device(PowerDevice.Kind.CONSUMER, Vector3(3.2,0.0,37.2), "Workshop Floodlight", 0.65)
    grid.connect_devices(generator, battery)
    grid.connect_devices(battery, light)

func _spawn_power_device(kind: PowerDevice.Kind, pos: Vector3, name: String, value_a: float, value_b := 0.0) -> PowerDevice:
    var device := PowerDevice.new()
    device.configure(kind, name, value_a, value_b)
    device.position = pos
    add_child(device)
    grid.register_device(device)
    return device

func _make_player() -> void:
    player = RiftPlayer.new()
    player.name = "Player"
    player.position = START_POSITION
    add_child(player)
    player.build_requested.connect(_on_build_requested)
    player.died.connect(_on_player_died)

func _populate_salvage() -> void:
    var types := [
        ["scrap", "Machined Scrap", 3, Color("70777d")],
        ["battery_cell", "Battery Cell", 1, Color("b45945")],
        ["cable", "Cable Coil", 1, Color("363b40")],
        ["electronics", "Control Board", 1, Color("6b8d74")],
        ["motor", "Industrial Motor", 1, Color("8c765a")],
        ["fuel", "Fuel Can", 1, Color("a06242")]
    ]
    for i in range(36):
        var data: Array = types[rng.randi_range(0, types.size()-1)]
        var p := Vector3(rng.randf_range(-68,68), 0.55, rng.randf_range(-74,74))
        if p.distance_to(START_POSITION) < 9:
            p.z -= 18
        _spawn_salvage(data[0], data[1], data[2], p, 1 if data[0] == "motor" else 0, data[3])

    for i in range(10):
        var data2: Array = types[rng.randi_range(1, types.size()-1)]
        var p2 := Vector3(-40 + rng.randf_range(-10,10), -16.7, -89 + rng.randf_range(-11,11))
        _spawn_salvage(data2[0], "Deep %s" % data2[1], data2[2], p2, 0, Color("7860a0") if data2[0] == "electronics" else data2[3])

func _spawn_salvage(item_id: String, name: String, amount: int, pos: Vector3, mass_class := 0, color := Color("8b949c")) -> void:
    var prop := SalvageProp.new()
    prop.configure(item_id, name, amount, mass_class, color)
    prop.position = pos
    add_child(prop)

func _spawn_humanoids() -> void:
    var positions := [Vector3(28,0.1,-34), Vector3(-16,0.1,-49), Vector3(51,0.1,18), Vector3(-58,0.1,51), Vector3(18,0.1,64), Vector3(42,0.1,-69)]
    for pos in positions:
        var enemy := HumanoidEnemy.new()
        enemy.position = pos
        enemy.target = player
        enemy.killed.connect(_on_enemy_killed)
        add_child(enemy)

func _spawn_colossus() -> void:
    var colossus := ColossusWalker.new()
    colossus.name = "WalkerColossus"
    colossus.position = Vector3(79,0,-65)
    colossus.route_radius = 22.0
    add_child(colossus)

func _on_build_requested(kind: String, pos: Vector3) -> void:
    var device: PowerDevice
    match kind:
        "floodlight":
            device = _spawn_power_device(PowerDevice.Kind.CONSUMER, pos, "Field Floodlight", 0.65)
        "generator":
            device = _spawn_power_device(PowerDevice.Kind.GENERATOR, pos + Vector3.UP * 0.58, "Field Generator", 3.2)
        "battery":
            device = _spawn_power_device(PowerDevice.Kind.BATTERY, pos + Vector3.UP * 0.68, "Field Battery", 5.0, 2.8)
        _:
            return
    var connected := grid.auto_connect(device, 16.0)
    status_message = "%s deployed%s." % [device.display_name, " and linked to nearest grid node" if is_instance_valid(connected) else " as an isolated grid"]
    status_timer = 4.0

func _on_enemy_killed(pos: Vector3) -> void:
    _spawn_salvage("scrap", "Recovered Scrap", rng.randi_range(2,5), pos + Vector3.UP * 0.45)

func _on_player_died() -> void:
    status_message = "You blacked out. Infrastructure persisted; you returned to the starter shelter."
    status_timer = 6.0
    player.global_position = START_POSITION
    player.velocity = Vector3.ZERO
    player.health = 100.0

func _make_hud() -> void:
    var layer := CanvasLayer.new()
    layer.name = "HUD"
    add_child(layer)

    var panel := ColorRect.new()
    panel.position = Vector2(14, 14)
    panel.size = Vector2(760, 76)
    panel.color = Color(0.015,0.02,0.03,0.72)
    layer.add_child(panel)

    hud = Label.new()
    hud.position = Vector2(26, 21)
    hud.add_theme_font_size_override("font_size", 16)
    hud.add_theme_color_override("font_color", Color("d7e1e8"))
    layer.add_child(hud)

    var cross := Label.new()
    cross.text = "+"
    cross.position = Vector2(635, 344)
    cross.add_theme_font_size_override("font_size", 24)
    cross.add_theme_color_override("font_color", Color(0.9,0.93,0.95,0.68))
    layer.add_child(cross)

    prompt = Label.new()
    prompt.position = Vector2(450, 405)
    prompt.size = Vector2(420, 40)
    prompt.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    prompt.add_theme_font_size_override("font_size", 17)
    prompt.add_theme_color_override("font_color", Color("f0dcb4"))
    layer.add_child(prompt)

    objective = Label.new()
    objective.position = Vector2(18, 678)
    objective.size = Vector2(1240, 32)
    objective.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    objective.add_theme_font_size_override("font_size", 15)
    objective.add_theme_color_override("font_color", Color("a9b8c4"))
    layer.add_child(objective)
