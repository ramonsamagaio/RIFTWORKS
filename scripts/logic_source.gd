class_name LogicSource
extends StaticBody3D

signal output_changed(value: bool)

enum SourceKind { BUTTON, PROXIMITY_SENSOR }

var source_kind := SourceKind.BUTTON
var output := false
var sensor_area: Area3D
var indicator: OmniLight3D
var occupants := 0

func configure(kind: SourceKind) -> void:
    source_kind = kind

func _ready() -> void:
    add_to_group("logic_sources")
    _build_visual()
    _build_collision()
    if source_kind == SourceKind.PROXIMITY_SENSOR:
        _build_sensor()
    _refresh()

func _mat(color: Color, metallic := 0.4, roughness := 0.58) -> StandardMaterial3D:
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

func _build_visual() -> void:
    if source_kind == SourceKind.BUTTON:
        _box(Vector3(0,0.35,0), Vector3(0.52,0.7,0.32), Color("30383d"))
        _box(Vector3(0,0.42,-0.19), Vector3(0.26,0.2,0.12), Color("824941"))
    else:
        _box(Vector3(0,0.4,0), Vector3(0.48,0.8,0.48), Color("30363c"))
        _box(Vector3(0,0.78,0), Vector3(0.72,0.12,0.72), Color("4a535a"))
    indicator = OmniLight3D.new()
    indicator.position = Vector3(0,0.72,-0.3)
    indicator.omni_range = 1.8
    indicator.light_energy = 0.55
    indicator.light_volumetric_fog_energy = 0.0
    add_child(indicator)

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(0.75,1.0,0.75)
    cs.shape = shape
    cs.position.y = 0.45
    add_child(cs)

func _build_sensor() -> void:
    sensor_area = Area3D.new()
    sensor_area.collision_layer = 0
    sensor_area.collision_mask = 1
    var cs := CollisionShape3D.new()
    var sphere := SphereShape3D.new()
    sphere.radius = 4.5
    cs.shape = sphere
    sensor_area.add_child(cs)
    sensor_area.body_entered.connect(_on_body_entered)
    sensor_area.body_exited.connect(_on_body_exited)
    add_child(sensor_area)

func _on_body_entered(body: Node) -> void:
    if body.is_in_group("player") or body is HumanoidEnemy or body is DroneEnemy:
        occupants += 1
        _set_output(true)

func _on_body_exited(body: Node) -> void:
    if body.is_in_group("player") or body is HumanoidEnemy or body is DroneEnemy:
        occupants = maxi(0, occupants - 1)
        if occupants == 0:
            _set_output(false)

func _set_output(value: bool) -> void:
    if output == value:
        return
    output = value
    _refresh()
    output_changed.emit(output)

func _refresh() -> void:
    if not is_instance_valid(indicator): return
    indicator.light_color = Color("57d08a") if output else Color("70332f")

func get_prompt_text() -> String:
    if source_kind == SourceKind.BUTTON:
        return "[E] Logic Button  -> %s" % ("ON" if output else "OFF")
    return "Proximity Sensor  -> %s" % ("ACTIVE" if output else "CLEAR")

func interact(_player: Node) -> void:
    if source_kind == SourceKind.BUTTON:
        _set_output(not output)
