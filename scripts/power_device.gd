class_name PowerDevice
extends StaticBody3D

enum Kind { GENERATOR, BATTERY, CONSUMER }

var kind: Kind = Kind.CONSUMER
var display_name := "Power Device"
var generation_kw := 0.0
var consumption_kw := 0.0
var capacity_kwh := 0.0
var charge_kwh := 0.0
var priority := 5
var enabled := true
var powered := false
var grid: PowerGridSystem
var status_light: OmniLight3D
var work_light: SpotLight3D
var body_material: StandardMaterial3D

func configure(p_kind: Kind, p_name: String, value_a: float = 0.0, value_b: float = 0.0) -> void:
    kind = p_kind
    display_name = p_name
    match kind:
        Kind.GENERATOR:
            generation_kw = value_a
        Kind.BATTERY:
            capacity_kwh = value_a
            charge_kwh = clampf(value_b if value_b > 0.0 else value_a * 0.65, 0.0, capacity_kwh)
        Kind.CONSUMER:
            consumption_kw = value_a

func _ready() -> void:
    add_to_group("power_devices")
    _build_collision()
    _build_visual()
    _refresh_visual_state()

func _build_collision() -> void:
    var collision := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    match kind:
        Kind.GENERATOR:
            shape.size = Vector3(1.7, 1.1, 1.1)
        Kind.BATTERY:
            shape.size = Vector3(1.15, 1.35, 0.75)
        Kind.CONSUMER:
            shape.size = Vector3(0.8, 2.7, 0.8)
    collision.shape = shape
    add_child(collision)

func _mat(color: Color, metallic := 0.0, roughness := 0.72) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _mesh_box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, metallic := 0.0) -> MeshInstance3D:
    var mesh_instance := MeshInstance3D.new()
    mesh_instance.position = pos
    var box := BoxMesh.new()
    box.size = size
    box.material = _mat(color, metallic)
    mesh_instance.mesh = box
    mesh_instance.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    parent.add_child(mesh_instance)
    return mesh_instance

func _build_visual() -> void:
    var root := Node3D.new()
    root.name = "Visual"
    add_child(root)

    match kind:
        Kind.GENERATOR:
            _mesh_box(root, Vector3(0, 0, 0), Vector3(1.7, 1.0, 1.05), Color("34383d"), 0.65)
            _mesh_box(root, Vector3(0, 0.25, -0.55), Vector3(1.25, 0.32, 0.12), Color("171a1d"), 0.2)
            _mesh_box(root, Vector3(-0.58, -0.52, 0), Vector3(0.22, 0.22, 1.15), Color("161719"), 0.15)
            _mesh_box(root, Vector3(0.58, -0.52, 0), Vector3(0.22, 0.22, 1.15), Color("161719"), 0.15)
        Kind.BATTERY:
            _mesh_box(root, Vector3(0, 0, 0), Vector3(1.1, 1.3, 0.7), Color("252b31"), 0.45)
            _mesh_box(root, Vector3(-0.28, 0.7, 0), Vector3(0.18, 0.1, 0.22), Color("a34538"), 0.7)
            _mesh_box(root, Vector3(0.28, 0.7, 0), Vector3(0.18, 0.1, 0.22), Color("34393d"), 0.7)
        Kind.CONSUMER:
            _mesh_box(root, Vector3(0, 0.65, 0), Vector3(0.12, 2.5, 0.12), Color("41474d"), 0.75)
            _mesh_box(root, Vector3(0, 1.85, 0), Vector3(0.75, 0.42, 0.42), Color("252a2d"), 0.55)
            work_light = SpotLight3D.new()
            work_light.position = Vector3(0, 1.85, -0.24)
            work_light.rotation_degrees.x = -12.0
            work_light.light_color = Color(1.0, 0.82, 0.60)
            work_light.light_energy = 28.0
            work_light.light_specular = 1.0
            work_light.light_size = 0.12
            work_light.light_volumetric_fog_energy = 1.35
            work_light.spot_range = 25.0
            work_light.spot_angle = 38.0
            work_light.spot_attenuation = 1.45
            work_light.spot_angle_attenuation = 2.6
            work_light.shadow_enabled = true
            work_light.shadow_bias = 0.025
            work_light.shadow_normal_bias = 0.45
            root.add_child(work_light)

    status_light = OmniLight3D.new()
    status_light.position = Vector3(0, 0.72 if kind != Kind.CONSUMER else 1.86, -0.58 if kind == Kind.GENERATOR else -0.38)
    status_light.omni_range = 1.8
    status_light.light_energy = 0.7
    status_light.light_volumetric_fog_energy = 0.0
    root.add_child(status_light)

func _refresh_visual_state() -> void:
    if not is_instance_valid(status_light):
        return
    if not enabled:
        status_light.light_color = Color("6f1e1e")
        status_light.light_energy = 0.25
    elif kind == Kind.CONSUMER and not powered:
        status_light.light_color = Color("a06b22")
        status_light.light_energy = 0.35
    else:
        status_light.light_color = Color("4cd685")
        status_light.light_energy = 0.75
    if is_instance_valid(work_light):
        work_light.visible = enabled and powered

func set_powered(value: bool) -> void:
    if powered == value:
        return
    powered = value
    _refresh_visual_state()

func get_prompt_text() -> String:
    var state := "ON" if enabled else "OFF"
    match kind:
        Kind.GENERATOR:
            return "[E] %s  %s  %.1f kW" % [display_name, state, generation_kw]
        Kind.BATTERY:
            return "[E] %s  %s  %.2f / %.2f kWh" % [display_name, state, charge_kwh, capacity_kwh]
        Kind.CONSUMER:
            return "[E] %s  %s  %.1f kW" % [display_name, state, consumption_kw]
    return "[E] %s" % display_name

func interact(_player: Node) -> void:
    enabled = not enabled
    if kind == Kind.CONSUMER and not enabled:
        set_powered(false)
    _refresh_visual_state()
    if is_instance_valid(grid):
        grid.force_recompute()
