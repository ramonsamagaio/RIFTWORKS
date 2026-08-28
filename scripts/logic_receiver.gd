class_name LogicReceiver
extends StaticBody3D

enum ReceiverKind { SIGNAL_LAMP, ALARM_BEACON }

var receiver_kind := ReceiverKind.SIGNAL_LAMP
var signal_state := false
var lamp: SpotLight3D
var beacon: OmniLight3D
var indicator: OmniLight3D

func configure(kind: ReceiverKind) -> void:
    receiver_kind = kind

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()
    set_signal(false)

func _mat(color: Color, metallic := 0.35, roughness := 0.62) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _box(pos: Vector3, size: Vector3, color: Color) -> void:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color)
    mi.mesh = mesh
    add_child(mi)

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(0.8, 1.5, 0.8)
    cs.shape = shape
    cs.position.y = 0.7
    add_child(cs)

func _build_visual() -> void:
    _box(Vector3(0,0.65,0), Vector3(0.18,1.3,0.18), Color("394148"))
    indicator = OmniLight3D.new()
    indicator.position = Vector3(0,1.15,-0.17)
    indicator.omni_range = 1.3
    indicator.light_energy = 0.45
    indicator.light_volumetric_fog_energy = 0.0
    add_child(indicator)

    match receiver_kind:
        ReceiverKind.SIGNAL_LAMP:
            _box(Vector3(0,1.35,0), Vector3(0.65,0.32,0.42), Color("252b30"))
            lamp = SpotLight3D.new()
            lamp.position = Vector3(0,1.34,-0.25)
            lamp.rotation_degrees.x = -10.0
            lamp.light_color = Color(1.0,0.82,0.60)
            lamp.light_energy = 24.0
            lamp.light_size = 0.11
            lamp.light_volumetric_fog_energy = 1.1
            lamp.spot_range = 22.0
            lamp.spot_angle = 36.0
            lamp.spot_attenuation = 1.8
            lamp.spot_angle_attenuation = 2.7
            lamp.shadow_enabled = true
            add_child(lamp)
        ReceiverKind.ALARM_BEACON:
            _box(Vector3(0,1.35,0), Vector3(0.45,0.3,0.45), Color("5b2925"))
            beacon = OmniLight3D.new()
            beacon.position = Vector3(0,1.55,0)
            beacon.light_color = Color("ef5549")
            beacon.light_energy = 4.0
            beacon.omni_range = 10.0
            beacon.light_volumetric_fog_energy = 1.3
            add_child(beacon)

func set_signal(value: bool) -> void:
    signal_state = value
    if is_instance_valid(lamp): lamp.visible = value
    if is_instance_valid(beacon): beacon.visible = value
    if is_instance_valid(indicator):
        indicator.light_color = Color("55cf88") if value else Color("66312e")

func get_prompt_text() -> String:
    var type_name := "Signal Lamp" if receiver_kind == ReceiverKind.SIGNAL_LAMP else "Alarm Beacon"
    return "%s  <- %s" % [type_name, "HIGH" if signal_state else "LOW"]
