class_name UndergroundDressing
extends Node3D

var rng := RandomNumberGenerator.new()

func _ready() -> void:
    rng.seed = 443190
    await get_tree().process_frame
    await get_tree().process_frame
    var graph := get_parent().get_node_or_null("UndergroundGraph")
    if not is_instance_valid(graph):
        return
    for child in graph.get_children():
        if not child is Node3D or not child.name.begins_with("DepthRoom_"):
            continue
        var room := child as Node3D
        var parts := room.name.split("_")
        if parts.size() < 3:
            continue
        var row := int(parts[2])
        if row <= 1:
            _dress_metro(room)
        elif row <= 3:
            _dress_mine(room)
        elif row <= 5:
            _dress_cave(room)
        else:
            _dress_breach(room)

func _mat(color: Color, metallic := 0.0, roughness := 0.88, emission := Color.BLACK, emission_energy := 0.0) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    if emission_energy > 0.0:
        mat.emission_enabled = true
        mat.emission = emission
        mat.emission_energy_multiplier = emission_energy
    return mat

func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, metallic := 0.0) -> MeshInstance3D:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color, metallic)
    mi.mesh = mesh
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    parent.add_child(mi)
    return mi

func _cylinder(parent: Node3D, pos: Vector3, radius: float, height: float, color: Color, sides := 6, emission := Color.BLACK, emission_energy := 0.0) -> MeshInstance3D:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var mesh := CylinderMesh.new()
    mesh.top_radius = radius * 0.72
    mesh.bottom_radius = radius
    mesh.height = height
    mesh.radial_segments = sides
    mesh.material = _mat(color, 0.08, 0.84, emission, emission_energy)
    mi.mesh = mesh
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    parent.add_child(mi)
    return mi

func _dress_metro(room: Node3D) -> void:
    _box(room, Vector3(-1.05, -0.15, 0), Vector3(0.12, 0.10, 10.0), Color("34383d"), 0.72)
    _box(room, Vector3(1.05, -0.15, 0), Vector3(0.12, 0.10, 10.0), Color("34383d"), 0.72)
    for z in [-4.5, -2.25, 0.0, 2.25, 4.5]:
        _box(room, Vector3(0, -0.19, z), Vector3(3.0, 0.10, 0.18), Color("292522"), 0.08)
    if rng.randf() < 0.45:
        var sign := _box(room, Vector3(0, 2.7, -4.7), Vector3(2.8, 0.55, 0.08), Color("1c2730"), 0.18)
        var sign_mat := sign.mesh.surface_get_material(0) as StandardMaterial3D
        if is_instance_valid(sign_mat):
            sign_mat.emission_enabled = true
            sign_mat.emission = Color("4a7388")
            sign_mat.emission_energy_multiplier = 0.25

func _dress_mine(room: Node3D) -> void:
    for z in [-4.2, 0.0, 4.2]:
        _box(room, Vector3(-4.4, 2.0, z), Vector3(0.22, 4.0, 0.32), Color("3b332b"))
        _box(room, Vector3(4.4, 2.0, z), Vector3(0.22, 4.0, 0.32), Color("3b332b"))
        _box(room, Vector3(0, 3.82, z), Vector3(9.0, 0.24, 0.32), Color("3b332b"))
    if rng.randf() < 0.65:
        _box(room, Vector3(rng.randf_range(-2.8,2.8), 0.45, rng.randf_range(-2.8,2.8)), Vector3(1.6,0.9,1.2), Color("34383a"), 0.35)

func _dress_cave(room: Node3D) -> void:
    for i in range(9):
        var rock := _cylinder(room, Vector3(rng.randf_range(-5.0,5.0), rng.randf_range(0.0,0.45), rng.randf_range(-5.0,5.0)), rng.randf_range(0.35,1.0), rng.randf_range(0.7,2.2), Color("24282b"), rng.randi_range(5,7))
        rock.rotation_degrees = Vector3(rng.randf_range(-22,22), rng.randf_range(0,180), rng.randf_range(-22,22))
    if rng.randf() < 0.38:
        var wet := OmniLight3D.new()
        wet.position = Vector3(rng.randf_range(-3,3), 0.55, rng.randf_range(-3,3))
        wet.light_color = Color("31505d")
        wet.light_energy = 0.55
        wet.omni_range = 5.5
        wet.light_volumetric_fog_energy = 0.3
        room.add_child(wet)

func _dress_breach(room: Node3D) -> void:
    for i in range(rng.randi_range(2,5)):
        var crystal_color := Color("7455a5") if rng.randf() < 0.6 else Color("4c9299")
        var crystal := _cylinder(room, Vector3(rng.randf_range(-4.2,4.2), rng.randf_range(0.45,1.0), rng.randf_range(-4.2,4.2)), rng.randf_range(0.18,0.48), rng.randf_range(1.1,2.8), Color("403653"), rng.randi_range(5,6), crystal_color, 2.4)
        crystal.rotation_degrees.z = rng.randf_range(-25,25)
    if rng.randf() < 0.55:
        var glow := OmniLight3D.new()
        glow.position = Vector3(rng.randf_range(-2.6,2.6), 1.6, rng.randf_range(-2.6,2.6))
        glow.light_color = Color("7357a5")
        glow.light_energy = 1.7
        glow.omni_range = 8.0
        glow.light_volumetric_fog_energy = 0.85
        room.add_child(glow)
