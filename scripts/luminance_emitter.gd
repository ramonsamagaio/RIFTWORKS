class_name LuminanceEmitter
extends StaticBody3D

var enabled := true
var core_light: OmniLight3D
var beam: SpotLight3D
var pulse := 0.0

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()
    _refresh()

func _process(delta: float) -> void:
    pulse += delta
    if enabled and is_instance_valid(core_light):
        core_light.light_energy = 5.4 + sin(pulse * 2.7) * 0.35

func _mat(color: Color, metallic := 0.4, roughness := 0.35) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(0.95, 1.9, 0.95)
    cs.shape = shape
    cs.position.y = 0.95
    add_child(cs)

func _build_visual() -> void:
    var pedestal := MeshInstance3D.new()
    pedestal.position.y = 0.55
    var pedestal_mesh := CylinderMesh.new()
    pedestal_mesh.top_radius = 0.48
    pedestal_mesh.bottom_radius = 0.62
    pedestal_mesh.height = 1.1
    pedestal_mesh.radial_segments = 8
    pedestal_mesh.material = _mat(Color("323039"), 0.62, 0.58)
    pedestal.mesh = pedestal_mesh
    add_child(pedestal)

    var crystal := MeshInstance3D.new()
    crystal.position.y = 1.45
    var crystal_mesh := CylinderMesh.new()
    crystal_mesh.top_radius = 0.04
    crystal_mesh.bottom_radius = 0.28
    crystal_mesh.height = 1.15
    crystal_mesh.radial_segments = 6
    var crystal_mat := _mat(Color("a58b55"), 0.05, 0.2)
    crystal_mat.emission_enabled = true
    crystal_mat.emission = Color("ffe0a1")
    crystal_mat.emission_energy_multiplier = 4.2
    crystal_mesh.material = crystal_mat
    crystal.mesh = crystal_mesh
    add_child(crystal)

    core_light = OmniLight3D.new()
    core_light.position = Vector3(0, 1.45, 0)
    core_light.light_color = Color("ffd99a")
    core_light.light_energy = 5.4
    core_light.omni_range = 15.0
    core_light.light_size = 0.32
    core_light.shadow_enabled = true
    core_light.light_volumetric_fog_energy = 1.5
    add_child(core_light)

    beam = SpotLight3D.new()
    beam.position = Vector3(0, 1.55, -0.18)
    beam.rotation_degrees.x = -20.0
    beam.light_color = Color("fff0c8")
    beam.light_energy = 23.0
    beam.spot_range = 32.0
    beam.spot_angle = 37.0
    beam.spot_attenuation = 1.5
    beam.spot_angle_attenuation = 2.8
    beam.light_size = 0.18
    beam.shadow_enabled = true
    beam.light_volumetric_fog_energy = 1.35
    add_child(beam)

func _refresh() -> void:
    if is_instance_valid(core_light):
        core_light.visible = enabled
    if is_instance_valid(beam):
        beam.visible = enabled

func set_signal(value: bool) -> void:
    enabled = value
    _refresh()

func get_prompt_text() -> String:
    return "[E] Luminance Core Lamp  %s  |  high-output Breach light" % ("ACTIVE" if enabled else "OFF")

func interact(_player: Node) -> void:
    enabled = not enabled
    _refresh()
