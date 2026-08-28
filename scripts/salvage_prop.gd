class_name SalvageProp
extends RigidBody3D

var item_id := "scrap"
var display_name := "Salvage"
var amount := 1
var mass_class := 0
var accent := Color("8b949c")

func configure(p_item_id: String, p_name: String, p_amount: int, p_mass_class := 0, p_color := Color("8b949c")) -> void:
    item_id = p_item_id
    display_name = p_name
    amount = p_amount
    mass_class = p_mass_class
    accent = p_color

func _ready() -> void:
    add_to_group("salvage")
    mass = 1.0 if mass_class <= 0 else 28.0
    freeze = mass_class <= 0
    continuous_cd = mass_class > 0
    linear_damp = 1.25 if mass_class > 0 else 0.0
    angular_damp = 2.2 if mass_class > 0 else 0.0
    _build_visual()
    _build_collision()

func _mat(color: Color, metallic := 0.0, roughness := 0.7) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _set_gi(mi: MeshInstance3D) -> void:
    mi.gi_mode = GeometryInstance3D.GI_MODE_DYNAMIC if mass_class > 0 else GeometryInstance3D.GI_MODE_STATIC

func _build_visual() -> void:
    match item_id:
        "motor":
            var motor := MeshInstance3D.new()
            var cyl := CylinderMesh.new()
            cyl.top_radius = 0.32
            cyl.bottom_radius = 0.32
            cyl.height = 0.75
            cyl.radial_segments = 10
            cyl.material = _mat(Color("31363b"), 0.75, 0.42)
            motor.mesh = cyl
            motor.rotation_degrees.z = 90.0
            _set_gi(motor)
            add_child(motor)
            _add_box(Vector3(0.42, 0, 0), Vector3(0.15, 0.28, 0.28), accent, 0.8)
        "battery_cell":
            _add_box(Vector3.ZERO, Vector3(0.42, 0.72, 0.28), Color("242b30"), 0.45)
            _add_box(Vector3(0, 0.38, 0), Vector3(0.16, 0.07, 0.13), accent, 0.6)
        "cable":
            var ring := MeshInstance3D.new()
            var torus := TorusMesh.new()
            torus.inner_radius = 0.18
            torus.outer_radius = 0.42
            torus.rings = 12
            torus.ring_segments = 8
            torus.material = _mat(Color("111418"), 0.05, 0.92)
            ring.mesh = torus
            ring.rotation_degrees.x = 90.0
            _set_gi(ring)
            add_child(ring)
        "electronics":
            _add_box(Vector3.ZERO, Vector3(0.7, 0.18, 0.52), Color("27332d"), 0.25)
            _add_box(Vector3(0.18, 0.12, -0.08), Vector3(0.16, 0.08, 0.16), accent, 0.45)
        "fuel":
            _add_box(Vector3.ZERO, Vector3(0.48, 0.72, 0.34), Color("4b3930"), 0.2)
            _add_box(Vector3(0.13, 0.4, 0), Vector3(0.13, 0.09, 0.11), accent, 0.55)
        "breach_core":
            var crystal := MeshInstance3D.new()
            crystal.position.y = 0.06
            crystal.rotation_degrees = Vector3(0,18,7)
            var crystal_mesh := CylinderMesh.new()
            crystal_mesh.top_radius = 0.08
            crystal_mesh.bottom_radius = 0.34
            crystal_mesh.height = 0.92
            crystal_mesh.radial_segments = 6
            var crystal_mat := _mat(Color("62507d"), 0.16, 0.28)
            crystal_mat.emission_enabled = true
            crystal_mat.emission = accent
            crystal_mat.emission_energy_multiplier = 2.8
            crystal_mesh.material = crystal_mat
            crystal.mesh = crystal_mesh
            _set_gi(crystal)
            add_child(crystal)
            _add_box(Vector3(0,-0.37,0), Vector3(0.72,0.16,0.72), Color("282b35"), 0.6)
            var glow := OmniLight3D.new()
            glow.position = Vector3(0,0.1,0)
            glow.light_color = accent
            glow.light_energy = 2.2
            glow.omni_range = 5.0
            glow.light_volumetric_fog_energy = 0.8
            add_child(glow)
        _:
            _add_box(Vector3.ZERO, Vector3(0.52, 0.32, 0.48), accent, 0.45)

func _add_box(pos: Vector3, size: Vector3, color: Color, metallic := 0.0) -> void:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var box := BoxMesh.new()
    box.size = size
    box.material = _mat(color, metallic)
    mi.mesh = box
    _set_gi(mi)
    add_child(mi)

func _build_collision() -> void:
    var collision := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(0.9, 0.9, 0.9) if mass_class == 0 else Vector3(1.3, 1.1, 1.2)
    collision.shape = shape
    add_child(collision)

func get_prompt_text() -> String:
    if mass_class <= 0:
        return "[E] Take %s  x%d" % [display_name, amount]
    return "[E] Lift %s  x%d  |  %.0f kg cargo" % [display_name, amount, mass]

func interact(player: Node) -> void:
    if mass_class > 0:
        var carry_nodes: Array[Node] = get_tree().get_nodes_in_group("carry_system")
        if not carry_nodes.is_empty() and carry_nodes[0].has_method("pickup_heavy"):
            carry_nodes[0].call("pickup_heavy", self)
        return
    if player.has_method("add_component"):
        player.add_component(item_id, amount)
        queue_free()
