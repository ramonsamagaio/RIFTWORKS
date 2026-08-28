class_name GravityEmitter
extends StaticBody3D

var enabled := true
var radius := 8.0
var field: Area3D
var core_light: OmniLight3D
var affected: Dictionary = {}
var pulse := 0.0

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()
    _build_field()

func _physics_process(delta: float) -> void:
    pulse += delta
    if is_instance_valid(core_light):
        core_light.light_energy = (3.8 + sin(pulse * 2.25) * 0.5) if enabled else 0.25
    if not enabled:
        _restore_all()
        return
    if not is_instance_valid(field):
        return
    for body in field.get_overlapping_bodies():
        if not body is RigidBody3D:
            continue
        var rigid := body as RigidBody3D
        var id := rigid.get_instance_id()
        if not affected.has(id):
            affected[id] = {"body": rigid, "gravity_scale": rigid.gravity_scale}
        rigid.gravity_scale = 0.12
        # Slight lift keeps very heavy cargo from feeling glued to the floor while preserving inertia.
        rigid.apply_central_force(Vector3.UP * rigid.mass * 2.6)

func _mat(color: Color, metallic := 0.45, roughness := 0.38) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := CylinderShape3D.new()
    shape.radius = 0.78
    shape.height = 1.15
    cs.shape = shape
    cs.position.y = 0.58
    add_child(cs)

func _build_visual() -> void:
    var base := MeshInstance3D.new()
    base.position.y = 0.48
    var base_mesh := CylinderMesh.new()
    base_mesh.top_radius = 0.68
    base_mesh.bottom_radius = 0.82
    base_mesh.height = 0.96
    base_mesh.radial_segments = 8
    base_mesh.material = _mat(Color("2e3038"), 0.7, 0.48)
    base.mesh = base_mesh
    add_child(base)

    var ring := MeshInstance3D.new()
    ring.position.y = 1.12
    ring.rotation_degrees.x = 90.0
    var torus := TorusMesh.new()
    torus.inner_radius = 0.22
    torus.outer_radius = 0.54
    torus.rings = 12
    torus.ring_segments = 8
    var ring_mat := _mat(Color("5b5875"), 0.2, 0.24)
    ring_mat.emission_enabled = true
    ring_mat.emission = Color("8b91e8")
    ring_mat.emission_energy_multiplier = 3.0
    torus.material = ring_mat
    ring.mesh = torus
    add_child(ring)

    core_light = OmniLight3D.new()
    core_light.position = Vector3(0,1.15,0)
    core_light.light_color = Color("8d98f0")
    core_light.light_energy = 3.8
    core_light.omni_range = 9.5
    core_light.light_volumetric_fog_energy = 1.1
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
    field.body_exited.connect(_on_body_exited)

func _on_body_exited(body: Node3D) -> void:
    if not body is RigidBody3D:
        return
    _restore_body(body as RigidBody3D)

func _restore_body(body: RigidBody3D) -> void:
    if not is_instance_valid(body):
        return
    var id := body.get_instance_id()
    if not affected.has(id):
        return
    var record: Dictionary = affected[id] as Dictionary
    body.gravity_scale = float(record.get("gravity_scale", 1.0))
    affected.erase(id)

func _restore_all() -> void:
    for id_value: Variant in affected.keys():
        var record: Dictionary = affected[id_value] as Dictionary
        var body := record.get("body") as RigidBody3D
        if is_instance_valid(body):
            body.gravity_scale = float(record.get("gravity_scale",1.0))
    affected.clear()

func set_signal(value: bool) -> void:
    enabled = value
    if not enabled:
        _restore_all()

func get_prompt_text() -> String:
    return "[E] Gravity Field  %s  |  reduces weight of physics cargo" % ("ACTIVE" if enabled else "OFF")

func interact(_player: Node) -> void:
    set_signal(not enabled)

func _exit_tree() -> void:
    _restore_all()
