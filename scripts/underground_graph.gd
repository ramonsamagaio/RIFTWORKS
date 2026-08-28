class_name UndergroundGraph
extends Node3D

const COLS := 6
const ROWS := 8
const CELL := 12.0
const START_CELL := Vector2i(2, 0)
const ORIGIN := Vector3(-70.0, -18.2, -112.0)
const SEED := 884211

var player: RiftPlayer
var rng := RandomNumberGenerator.new()
var links: Dictionary = {}

func _ready() -> void:
    rng.seed = SEED
    links = _generate_maze()
    _build_maze()
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _populate()

func _mat(color: Color, metallic: float = 0.0, roughness: float = 0.9) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, collide: bool = true, metallic: float = 0.0) -> Node3D:
    var root: Node3D
    if collide:
        var body := StaticBody3D.new()
        root = body
        var cs := CollisionShape3D.new()
        var shape := BoxShape3D.new()
        shape.size = size
        cs.shape = shape
        body.add_child(cs)
    else:
        root = Node3D.new()
    root.position = pos
    var mi := MeshInstance3D.new()
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color, metallic)
    mi.mesh = mesh
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    root.add_child(mi)
    parent.add_child(root)
    return root

func _generate_maze() -> Dictionary:
    var result: Dictionary = {}
    var visited: Dictionary = {}
    var stack: Array[Vector2i] = [START_CELL]
    visited[START_CELL] = true
    result[START_CELL] = []
    var dirs: Array[Vector2i] = [Vector2i(1,0), Vector2i(-1,0), Vector2i(0,1), Vector2i(0,-1)]

    while not stack.is_empty():
        var current: Vector2i = stack[-1]
        var candidates: Array[Vector2i] = []
        for dir: Vector2i in dirs:
            var next_cell: Vector2i = current + dir
            if next_cell.x < 0 or next_cell.x >= COLS or next_cell.y < 0 or next_cell.y >= ROWS:
                continue
            if not visited.has(next_cell):
                candidates.append(next_cell)
        if candidates.is_empty():
            stack.pop_back()
            continue
        var chosen: Vector2i = candidates[rng.randi_range(0, candidates.size()-1)]
        if not result.has(current):
            result[current] = []
        if not result.has(chosen):
            result[chosen] = []
        var current_links: Array = result[current] as Array
        var chosen_links: Array = result[chosen] as Array
        current_links.append(chosen)
        chosen_links.append(current)
        visited[chosen] = true
        stack.append(chosen)

    # Add a few loops so the underworld is not a single perfect maze.
    for _i: int in range(10):
        var c := Vector2i(rng.randi_range(0,COLS-1), rng.randi_range(0,ROWS-1))
        var dir: Vector2i = dirs[rng.randi_range(0,dirs.size()-1)]
        var n: Vector2i = c + dir
        if n.x >= 0 and n.x < COLS and n.y >= 0 and n.y < ROWS:
            var c_links: Array = result.get(c, []) as Array
            var n_links: Array = result.get(n, []) as Array
            if not n in c_links:
                c_links.append(n)
                n_links.append(c)
                result[c] = c_links
                result[n] = n_links
    return result

func _cell_position(cell: Vector2i) -> Vector3:
    return ORIGIN + Vector3(cell.x * CELL, 0, -cell.y * CELL)

func _connected(a: Vector2i, b: Vector2i) -> bool:
    if not links.has(a):
        return false
    var a_links: Array = links[a] as Array
    return b in a_links

func _build_maze() -> void:
    for x: int in range(COLS):
        for y: int in range(ROWS):
            var cell := Vector2i(x,y)
            var root := Node3D.new()
            root.name = "DepthRoom_%d_%d" % [x,y]
            root.position = _cell_position(cell)
            add_child(root)

            var tint: float = rng.randf_range(-0.01,0.018)
            _box(root, Vector3(0,-0.55,0), Vector3(CELL,0.55,CELL), Color(0.07+tint,0.078+tint,0.087+tint))
            _box(root, Vector3(0,4.65,0), Vector3(CELL,0.35,CELL), Color("151a1f"))

            _wall_x(root, Vector3(0,2.0,-CELL*0.5), _connected(cell, cell + Vector2i(0,-1)) or cell == START_CELL)
            _wall_x(root, Vector3(0,2.0,CELL*0.5), _connected(cell, cell + Vector2i(0,1)))
            _wall_z(root, Vector3(-CELL*0.5,2.0,0), _connected(cell, cell + Vector2i(-1,0)))
            _wall_z(root, Vector3(CELL*0.5,2.0,0), _connected(cell, cell + Vector2i(1,0)))

            if rng.randf() < 0.28:
                _box(root, Vector3(rng.randf_range(-3.2,3.2),0.35,rng.randf_range(-3.2,3.2)), Vector3(rng.randf_range(1.6,3.2),0.7,rng.randf_range(1.4,2.8)), Color("252b30"), true, 0.35)

func _wall_x(root: Node3D, center: Vector3, opening: bool) -> void:
    if not opening:
        _box(root, center, Vector3(CELL,4.8,0.35), Color("20262b"))
        return
    _box(root, center + Vector3(-4.0,0,0), Vector3(4.0,4.8,0.35), Color("20262b"))
    _box(root, center + Vector3(4.0,0,0), Vector3(4.0,4.8,0.35), Color("20262b"))
    _box(root, center + Vector3(0,1.7,0), Vector3(4.0,1.4,0.35), Color("20262b"))

func _wall_z(root: Node3D, center: Vector3, opening: bool) -> void:
    if not opening:
        _box(root, center, Vector3(0.35,4.8,CELL), Color("20262b"))
        return
    _box(root, center + Vector3(0,0,-4.0), Vector3(0.35,4.8,4.0), Color("20262b"))
    _box(root, center + Vector3(0,0,4.0), Vector3(0.35,4.8,4.0), Color("20262b"))
    _box(root, center + Vector3(0,1.7,0), Vector3(0.35,1.4,4.0), Color("20262b"))

func _populate() -> void:
    if not is_instance_valid(player):
        return
    for x: int in range(COLS):
        for y: int in range(ROWS):
            var cell := Vector2i(x,y)
            var base: Vector3 = _cell_position(cell)
            if rng.randf() < 0.17 and cell != START_CELL:
                var drone := DroneEnemy.new()
                drone.target = player
                drone.position = base + Vector3(rng.randf_range(-2.5,2.5), 2.2, rng.randf_range(-2.5,2.5))
                add_child(drone)
            if rng.randf() < 0.24:
                var prop := SalvageProp.new()
                var breach_roll: float = rng.randf()
                if breach_roll < 0.2:
                    prop.configure("electronics", "Breach Circuit", 2, 0, Color("8e6cc4"))
                elif breach_roll < 0.52:
                    prop.configure("battery_cell", "Deep Battery Cell", 1, 0, Color("a05362"))
                else:
                    prop.configure("scrap", "Underground Alloy", rng.randi_range(2,5), 0, Color("6b707c"))
                prop.position = base + Vector3(rng.randf_range(-3,3),0.5,rng.randf_range(-3,3))
                add_child(prop)
            if rng.randf() < 0.10:
                _breach_marker(base + Vector3(rng.randf_range(-2,2),0,rng.randf_range(-2,2)))

func _breach_marker(pos: Vector3) -> void:
    var root := Node3D.new()
    root.position = pos
    add_child(root)
    var crystal := MeshInstance3D.new()
    crystal.position.y = 1.0
    crystal.rotation_degrees.z = rng.randf_range(-15,15)
    var mesh := CylinderMesh.new()
    mesh.top_radius = 0.08
    mesh.bottom_radius = 0.42
    mesh.height = 2.1
    mesh.radial_segments = 5
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("4d4164")
    mat.emission_enabled = true
    mat.emission = Color("7756b8")
    mat.emission_energy_multiplier = 2.2
    mat.roughness = 0.32
    mesh.material = mat
    crystal.mesh = mesh
    root.add_child(crystal)
    var light := OmniLight3D.new()
    light.position.y = 1.2
    light.light_color = Color("8060c7")
    light.light_energy = 2.6
    light.omni_range = 8.0
    light.light_volumetric_fog_energy = 1.25
    root.add_child(light)
