class_name AttractionEmitter
extends StaticBody3D

var enabled := true
var radius := 8.5
var force_strength := 1180.0
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
        core_light.light_energy = (3.2 + sin(pulse * 3.8) * 0.55) if enabled else 0.3
    if not enabled or not is_instance_valid(field):
        return
    for body in field.get_overlapping_bodies():
        if body is RigidBody3D:
            var rigid := body as RigidBody3D
            var delta_pos := global_position - rigid.global_position
            var distance := maxf(0.85, delta_pos.length())
            var falloff := clampf(1.0 - distance / radius, 0.0, 1.0)
            rigid.apply_central_force(delta_pos.normalized() * force_strength * falloff * falloff)

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
    base_mesh.material = _mat(Color("262f35"), 0.65, 0.55)
    base.mesh = base_mesh
    add_child(base)

    var crystal := MeshInstance3D.new()
    crystal.position.y = 1.22
    crystal.rotation_degrees.z = -9.0
    var crystal_mesh := CylinderMesh.new()
    crystal_mesh.top_radius = 0.08
    crystal_mesh.bottom_radius = 0.34
    crystal_mesh.height = 1.15
    crystal_mesh.radial_segments = 6
    var crystal_mat := _mat(Color("416d77"), 0.12, 0.24)
    crystal_mat.emission_enabled = true
    crystal_mat.emission = Color("55c0c8")
    crystal_mat.emission_energy_multiplier = 3.2
    crystal_mesh.material = crystal_mat
    crystal.mesh = crystal_mesh
    add_child(crystal)

    core_light = OmniLight3D.new()
    core_light.position = Vector3(0,1.15,0)
    core_light.light_color = Color("55c0c8")
    core_light.light_energy = 3.5
    core_light.omni_range = 8.5
    core_light.light_volumetric_fog_energy = 1.35
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
    return "[E] Attraction Emitter  %s  |  pulls physics parts" % ("ACTIVE" if enabled else "OFF")

func interact(_player: Node) -> void:
    enabled = not enabled
