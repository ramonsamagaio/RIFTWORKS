class_name ProceduralWorldStreamer
extends Node3D

const CHUNK_SIZE := 64.0
const ACTIVE_RADIUS := 2
const CORE_RADIUS := 112.0
const WORLD_SEED := 731942

var player: RiftPlayer
var chunks := {}
var refresh_timer := 0.0
var region_noise := FastNoiseLite.new()

func _ready() -> void:
    region_noise.seed = WORLD_SEED
    region_noise.frequency = 0.0038
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _refresh_chunks()

func _process(delta: float) -> void:
    refresh_timer -= delta
    if refresh_timer <= 0.0:
        refresh_timer = 0.65
        _refresh_chunks()

func _refresh_chunks() -> void:
    if not is_instance_valid(player):
        return
    var center := Vector2i(floori(player.global_position.x / CHUNK_SIZE), floori(player.global_position.z / CHUNK_SIZE))
    var wanted := {}
    for x in range(center.x - ACTIVE_RADIUS, center.x + ACTIVE_RADIUS + 1):
        for z in range(center.y - ACTIVE_RADIUS, center.y + ACTIVE_RADIUS + 1):
            var key := Vector2i(x,z)
            var world_center := Vector3((x + 0.5) * CHUNK_SIZE, 0, (z + 0.5) * CHUNK_SIZE)
            if Vector2(world_center.x, world_center.z).length() < CORE_RADIUS:
                continue
            wanted[key] = true
            if not chunks.has(key):
                chunks[key] = _generate_chunk(key)

    for key in chunks.keys().duplicate():
        if not wanted.has(key):
            var node: Node = chunks[key]
            if is_instance_valid(node):
                node.queue_free()
            chunks.erase(key)

func _seed_for(key: Vector2i) -> int:
    return abs(WORLD_SEED ^ (key.x * 92837111) ^ (key.y * 689287499))

func _region_value(key: Vector2i) -> float:
    var wx := (key.x + 0.5) * CHUNK_SIZE
    var wz := (key.y + 0.5) * CHUNK_SIZE
    return (region_noise.get_noise_2d(wx,wz) + 1.0) * 0.5

func _mat(color: Color, metallic := 0.0, roughness := 0.88) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, collide := true, metallic := 0.0) -> Node3D:
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

func _generate_chunk(key: Vector2i) -> Node3D:
    var root := Node3D.new()
    root.name = "SurfaceChunk_%d_%d" % [key.x,key.y]
    root.position = Vector3(key.x * CHUNK_SIZE, 0, key.y * CHUNK_SIZE)
    add_child(root)

    var local_rng := RandomNumberGenerator.new()
    local_rng.seed = _seed_for(key)
    var region := _region_value(key)
    var ground_tint := lerpf(-0.008,0.012,region) + local_rng.randf_range(-0.004,0.004)
    _box(root, Vector3(CHUNK_SIZE*0.5,-0.55,CHUNK_SIZE*0.5), Vector3(CHUNK_SIZE+0.1,1,CHUNK_SIZE+0.1), Color(0.052+ground_tint,0.061+ground_tint,0.068+ground_tint))

    var has_x_road := posmod(key.y, 3) == 0
    var has_z_road := posmod(key.x, 3) == 0
    if has_x_road:
        _box(root, Vector3(CHUNK_SIZE*0.5,0.02,CHUNK_SIZE*0.5), Vector3(CHUNK_SIZE,0.05,8), Color("14181d"), false)
    if has_z_road:
        _box(root, Vector3(CHUNK_SIZE*0.5,0.022,CHUNK_SIZE*0.5), Vector3(8,0.05,CHUNK_SIZE), Color("14181d"), false)

    # Continuous region noise deliberately creates broad transition belts instead of hard biome chunks.
    if region < 0.34:
        _generate_woodland(root, local_rng, 1.0)
    elif region < 0.44:
        _generate_woodland(root, local_rng, 0.68)
        _generate_sparse_urban(root, local_rng, has_x_road, has_z_road, 0.32)
    elif region < 0.62:
        _generate_woodland(root, local_rng, 0.18)
        _generate_sparse_urban(root, local_rng, has_x_road, has_z_road, 0.82)
    elif region < 0.73:
        _generate_sparse_urban(root, local_rng, has_x_road, has_z_road, 0.55)
        _generate_industrial(root, local_rng, 0.45)
    else:
        _generate_industrial(root, local_rng, 1.0)

    _spawn_chunk_salvage(root, local_rng)
    return root

func _generate_sparse_urban(root: Node3D, r: RandomNumberGenerator, road_x: bool, road_z: bool, density := 1.0) -> void:
    var count := maxi(1,roundi(r.randi_range(4,8) * density))
    for i in range(count):
        var x := r.randf_range(7,57)
        var z := r.randf_range(7,57)
        if road_x and abs(z-32) < 8: continue
        if road_z and abs(x-32) < 8: continue
        var w := r.randf_range(7,13)
        var d := r.randf_range(7,13)
        var h := r.randf_range(3.5,10.5)
        var tint := r.randf_range(0.0,0.028)
        _box(root, Vector3(x,h*0.5,z), Vector3(w,h,d), Color(0.074+tint,0.079+tint,0.083+tint*0.8))

func _generate_industrial(root: Node3D, r: RandomNumberGenerator, density := 1.0) -> void:
    var count := maxi(1,roundi(r.randi_range(2,4) * density))
    for i in range(count):
        var x := r.randf_range(12,52)
        var z := r.randf_range(12,52)
        var w := r.randf_range(13,22)
        var d := r.randf_range(11,19)
        var h := r.randf_range(5,9)
        _box(root, Vector3(x,h*0.5,z), Vector3(w,h,d), Color("242a2e"), true, 0.18)
        _box(root, Vector3(x+r.randf_range(-4,4),h+0.6,z+r.randf_range(-4,4)), Vector3(3.2,1.0,2.4), Color("30363a"), true, 0.32)

func _generate_woodland(root: Node3D, r: RandomNumberGenerator, density := 1.0) -> void:
    var count := maxi(2,roundi(r.randi_range(18,34) * density))
    for i in range(count):
        var x := r.randf_range(3,61)
        var z := r.randf_range(3,61)
        _tree(root, Vector3(x,0,z), r.randf_range(0.8,1.45), r)

func _tree(parent: Node3D, pos: Vector3, scale_factor: float, r: RandomNumberGenerator) -> void:
    var root := Node3D.new()
    root.position = pos
    parent.add_child(root)

    var trunk_body := StaticBody3D.new()
    trunk_body.position = Vector3(0,1.6*scale_factor,0)
    var trunk_cs := CollisionShape3D.new()
    var trunk_shape := CylinderShape3D.new()
    trunk_shape.radius = 0.28*scale_factor
    trunk_shape.height = 3.2*scale_factor
    trunk_cs.shape = trunk_shape
    trunk_body.add_child(trunk_cs)
    var trunk_mesh := MeshInstance3D.new()
    var cylinder := CylinderMesh.new()
    cylinder.top_radius = 0.23*scale_factor
    cylinder.bottom_radius = 0.34*scale_factor
    cylinder.height = 3.2*scale_factor
    cylinder.radial_segments = 6
    cylinder.material = _mat(Color("302a26"),0.0,0.95)
    trunk_mesh.mesh = cylinder
    trunk_body.add_child(trunk_mesh)
    root.add_child(trunk_body)

    var crown := MeshInstance3D.new()
    crown.position = Vector3(0,3.6*scale_factor,0)
    var cone := CylinderMesh.new()
    cone.top_radius = 0.15*scale_factor
    cone.bottom_radius = r.randf_range(1.3,1.8)*scale_factor
    cone.height = 3.8*scale_factor
    cone.radial_segments = 7
    cone.material = _mat(Color(0.055,0.085+r.randf_range(0,0.025),0.068),0.0,0.96)
    crown.mesh = cone
    root.add_child(crown)

func _spawn_chunk_salvage(root: Node3D, r: RandomNumberGenerator) -> void:
    if r.randf() > 0.7:
        return
    var ids := ["scrap","battery_cell","cable","electronics","motor","fuel"]
    var names := ["Scrap","Battery Cell","Cable Coil","Control Board","Industrial Motor","Fuel Can"]
    var idx := r.randi_range(0,ids.size()-1)
    var prop := SalvageProp.new()
    prop.configure(ids[idx], names[idx], 1 if idx > 0 else r.randi_range(2,4), 1 if ids[idx] == "motor" else 0)
    prop.position = Vector3(r.randf_range(8,56),0.55,r.randf_range(8,56))
    root.add_child(prop)
