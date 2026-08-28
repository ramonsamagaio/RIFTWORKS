class_name RepulsionEmitter
extends StaticBody3D

var enabled := true
var radius := 7.5
var force_strength := 1450.0
var field: Area3D
var core_light: OmniLight3D
var pulse := 0.0

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()
    _build_field()

func _physics_process(delta: float) -> void:
    pulse += delta
    if is_instance_valid(core_light):
        core_light.light_energy = (3.8 + sin(pulse * 4.6) * 0.7) if enabled else 0.35
    if not enabled or not is_instance_valid(field):
        return
    for body in field.get_overlapping_bodies():
        if body is RigidBody3D:
            var delta_pos := body.global_position - global_position
            var distance := maxf(0.8, delta_pos.length())
            var falloff := clampf(1.0 - distance / radius, 0.0, 1.0)
            var impulse_force := delta_pos.normalized() * force_strength * falloff * falloff
            body.apply_central_force(impulse_force)

func _mat(color: Color, metallic := 0.45, roughness := 0.42) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := CylinderShape3D.new()
    shape.radius = 0.75
    shape.height = 1.25
    cs.shape = shape
    cs.position.y = 0.62
    add_child(cs)

func _build_visual() -> void:
    var base := MeshInstance3D.new()
    base.position.y = 0.42
    var base_mesh := CylinderMesh.new()
    base_mesh.top_radius = 0.72
    base_mesh.bottom_radius = 0.82
    base_mesh.height = 0.82
    base_mesh.radial_segments = 8
    base_mesh.material = _mat(Color("292d36"),0.65,0.55)
    base.mesh = base_mesh
    add_child(base)

    var crystal := MeshInstance3D.new()
    crystal.position.y = 1.22
    crystal.rotation_degrees.z = 10.0
    var crystal_mesh := CylinderMesh.new()
    crystal_mesh.top_radius = 0.08
    crystal_mesh.bottom_radius = 0.34
    crystal_mesh.height = 1.15
    crystal_mesh.radial_segments = 6
    var crystal_mat := _mat(Color("715d96"),0.12,0.24)
    crystal_mat.emission_enabled = true
    crystal_mat.emission = Color("9874db")
    crystal_mat.emission_energy_multiplier = 3.4
    crystal_mesh.material = crystal_mat
    crystal.mesh = crystal_mesh
    add_child(crystal)

    core_light = OmniLight3D.new()
    core_light.position = Vector3(0,1.15,0)
    core_light.light_color = Color("9874db")
    core_light.light_energy = 4.2
    core_light.omni_range = 9.0
    core_light.light_volumetric_fog_energy = 1.6
    add_child(core_light)

func _build_field() -> void:
    field = Area3D.new()
    field.collision_layer = 0
    field.collision_mask = 1
    field.monitoring = true
    var cs := CollisionShape3D.new()
    var sphere := SphereShape3D.new()
    sphere.radius = radius
    cs.shape = sphere
    field.add_child(cs)
    add_child(field)

func set_signal(value: bool) -> void:
    enabled = value

func get_prompt_text() -> String:
    return "[E] Repulsion Emitter  %s  |  pushes physics parts" % ("ACTIVE" if enabled else "OFF")

func interact(_player: Node) -> void:
    enabled = not enabled
