class_name WinchDevice
extends StaticBody3D

var enabled := false
var target_body: RigidBody3D
var pull_force := 1850.0
var max_range := 18.0
var stop_distance := 1.8
var cable: MeshInstance3D
var cable_mesh: CylinderMesh
var status_light: OmniLight3D

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()

func _physics_process(_delta: float) -> void:
    if not is_instance_valid(target_body):
        enabled = false
        _update_cable()
        return
    var delta_pos := global_position + Vector3.UP * 0.65 - target_body.global_position
    var distance := delta_pos.length()
    if distance > max_range * 1.35:
        release_target()
        return
    if enabled and distance > stop_distance:
        var force_scale := clampf(distance / max_range, 0.25, 1.0)
        target_body.apply_central_force(delta_pos.normalized() * pull_force * force_scale)
    _update_cable()

func _mat(color: Color, metallic := 0.55, roughness := 0.58) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(1.35, 1.15, 1.05)
    cs.shape = shape
    cs.position.y = 0.58
    add_child(cs)

func _build_visual() -> void:
    var housing := MeshInstance3D.new()
    housing.position.y = 0.58
    var box := BoxMesh.new()
    box.size = Vector3(1.35, 1.15, 1.05)
    box.material = _mat(Color("333a3f"), 0.68, 0.5)
    housing.mesh = box
    add_child(housing)

    var drum := MeshInstance3D.new()
    drum.position = Vector3(0,0.66,-0.62)
    drum.rotation_degrees.z = 90.0
    var drum_mesh := CylinderMesh.new()
    drum_mesh.top_radius = 0.34
    drum_mesh.bottom_radius = 0.34
    drum_mesh.height = 0.72
    drum_mesh.radial_segments = 10
    drum_mesh.material = _mat(Color("1d2226"), 0.82, 0.42)
    drum.mesh = drum_mesh
    add_child(drum)

    status_light = OmniLight3D.new()
    status_light.position = Vector3(0.45,1.05,-0.5)
    status_light.light_color = Color("d1a457")
    status_light.light_energy = 0.55
    status_light.omni_range = 2.4
    add_child(status_light)

    cable = MeshInstance3D.new()
    cable_mesh = CylinderMesh.new()
    cable_mesh.top_radius = 0.026
    cable_mesh.bottom_radius = 0.026
    cable_mesh.height = 0.1
    cable_mesh.radial_segments = 6
    cable_mesh.material = _mat(Color("15181b"), 0.42, 0.82)
    cable.mesh = cable_mesh
    cable.visible = false
    add_child(cable)

func acquire_nearest() -> bool:
    var best: RigidBody3D
    var best_distance := max_range
    for node in get_tree().get_nodes_in_group("assembly_parts"):
        if node is RigidBody3D:
            var rigid := node as RigidBody3D
            var distance := global_position.distance_to(rigid.global_position)
            if distance < best_distance:
                best_distance = distance
                best = rigid
    for node in get_tree().get_nodes_in_group("salvage"):
        if node is SalvageProp:
            var salvage := node as SalvageProp
            if salvage.mass_class <= 0:
                continue
            var distance := global_position.distance_to(salvage.global_position)
            if distance < best_distance:
                best_distance = distance
                best = salvage
    if not is_instance_valid(best):
        return false
    target_body = best
    enabled = true
    _refresh_state()
    return true

func release_target() -> void:
    target_body = null
    enabled = false
    _refresh_state()
    _update_cable()

func _update_cable() -> void:
    if not is_instance_valid(cable) or not is_instance_valid(target_body):
        if is_instance_valid(cable):
            cable.visible = false
        return
    var local_a := Vector3(0,0.68,-0.68)
    var world_a := global_position + local_a
    var world_b := target_body.global_position
    var delta := world_b - world_a
    var length := delta.length()
    if length < 0.05:
        cable.visible = false
        return
    cable.visible = true
    cable.global_position = (world_a + world_b) * 0.5
    cable_mesh.height = length
    cable.quaternion = Quaternion(Vector3.UP, delta.normalized())

func _refresh_state() -> void:
    if not is_instance_valid(status_light):
        return
    if not is_instance_valid(target_body):
        status_light.light_color = Color("9b6d45")
        status_light.light_energy = 0.35
    elif enabled:
        status_light.light_color = Color("6fd1a0")
        status_light.light_energy = 0.85
    else:
        status_light.light_color = Color("c7a75f")
        status_light.light_energy = 0.55

func set_signal(value: bool) -> void:
    if not is_instance_valid(target_body) and value:
        acquire_nearest()
        return
    enabled = value and is_instance_valid(target_body)
    _refresh_state()

func get_prompt_text() -> String:
    if not is_instance_valid(target_body):
        return "[E] Winch  |  acquire nearest heavy cargo / assembly part"
    return "[E] Winch  %s  |  target %s  |  SHIFT+E release" % [("PULLING" if enabled else "HOLD"), target_body.name]

func interact(_player: Node) -> void:
    if Input.is_key_pressed(KEY_SHIFT):
        release_target()
        return
    if not is_instance_valid(target_body):
        acquire_nearest()
    else:
        enabled = not enabled
        _refresh_state()
