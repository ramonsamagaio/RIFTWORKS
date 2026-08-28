class_name AssemblySystem
extends Node3D

const BLUEPRINT_PATH := "user://riftworks_last_blueprint.json"

var player: RiftPlayer
var motors: Array[LogicMotorJoint] = []
var motor_enabled := true
var ui: Label
var part_counter := 0

func _ready() -> void:
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _make_ui()

func _process(_delta: float) -> void:
    for child in get_children():
        if child is RigidBody3D and child.global_position.y < -90.0:
            child.queue_free()

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_1:
            _place_box_part("Platform", Vector3(2.6,0.28,2.2), 18.0, 2)
        KEY_2:
            _place_box_part("Beam", Vector3(0.32,0.32,3.0), 5.0, 1)
        KEY_3:
            _place_wheel(false)
        KEY_4:
            _place_wheel(true)
        KEY_5:
            _anchor_nearest()
        KEY_X:
            motor_enabled = not motor_enabled
            for motor in motors:
                if is_instance_valid(motor):
                    motor.set_signal(motor_enabled)
            _message("Assembly motors %s" % ("ON" if motor_enabled else "OFF"))
        KEY_O:
            _save_blueprint()
        KEY_P:
            _deploy_blueprint()

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    var bg := ColorRect.new()
    bg.position = Vector2(875, 566)
    bg.size = Vector2(390, 136)
    bg.color = Color(0.012,0.016,0.024,0.66)
    layer.add_child(bg)
    ui = Label.new()
    ui.position = Vector2(890, 578)
    ui.size = Vector2(360, 112)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("b9c8d2"))
    ui.text = "FAS PROTOTYPE\n1 platform  2 beam  3 wheel  4 logic motor wheel\n5 anchor  X motors  O save blueprint  P rebuild\nMotors can receive button/sensor signals."
    layer.add_child(ui)

func _build_position() -> Vector3:
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 8.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit := get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        return hit.position + hit.normal * 0.35
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0, 0.7, -3.5)

func _material(color: Color, metallic := 0.55, roughness := 0.58) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _place_box_part(label: String, size: Vector3, mass: float, scrap_cost: int) -> void:
    if player.scrap < scrap_cost:
        _message("Need %d scrap" % scrap_cost)
        return
    player.scrap -= scrap_cost
    _spawn_box_body(label, size, mass, _build_position(), Vector3.ZERO)
    _message("Placed %s. Physics is live." % label)

func _spawn_box_body(label: String, size: Vector3, mass: float, pos: Vector3, rotation: Vector3) -> RigidBody3D:
    var body := RigidBody3D.new()
    part_counter += 1
    body.name = "%s_%03d" % [label, part_counter]
    body.mass = mass
    body.global_position = pos
    body.global_rotation = rotation
    body.continuous_cd = true
    body.add_to_group("assembly_parts")
    body.set_meta("part_type", label)

    var mesh_instance := MeshInstance3D.new()
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _material(Color("58636a") if label == "Platform" else Color("475158"))
    mesh_instance.mesh = mesh
    body.add_child(mesh_instance)

    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = size
    cs.shape = shape
    body.add_child(cs)
    add_child(body)
    return body

func _place_wheel(powered_motor: bool) -> void:
    var scrap_cost := 3 if powered_motor else 2
    if player.scrap < scrap_cost:
        _message("Need %d scrap" % scrap_cost)
        return
    if powered_motor and int(player.components.get("motor", 0)) < 1:
        _message("Need an industrial motor")
        return
    player.scrap -= scrap_cost
    if powered_motor:
        player.components["motor"] = int(player.components.get("motor", 0)) - 1

    var pos := _build_position()
    var parent_body := _nearest_part(pos, 3.6)
    _spawn_wheel_body(powered_motor, pos, Vector3.ZERO, parent_body)

func _spawn_wheel_body(powered_motor: bool, pos: Vector3, rotation: Vector3, parent_body: RigidBody3D = null) -> RigidBody3D:
    var wheel := RigidBody3D.new()
    part_counter += 1
    var type_name := "MotorWheel" if powered_motor else "Wheel"
    wheel.name = type_name + "_%03d" % part_counter
    wheel.mass = 4.5
    wheel.global_position = pos
    wheel.global_rotation = rotation
    wheel.continuous_cd = true
    wheel.add_to_group("assembly_parts")
    wheel.set_meta("part_type", type_name)
    if is_instance_valid(parent_body):
        wheel.set_meta("parent_part_name", parent_body.name)

    var mesh_instance := MeshInstance3D.new()
    var mesh := CylinderMesh.new()
    mesh.top_radius = 0.62
    mesh.bottom_radius = 0.62
    mesh.height = 0.34
    mesh.radial_segments = 12
    mesh.material = _material(Color("20252a"), 0.18, 0.84)
    mesh_instance.mesh = mesh
    mesh_instance.rotation_degrees.z = 90.0
    wheel.add_child(mesh_instance)

    var cs := CollisionShape3D.new()
    var shape := CylinderShape3D.new()
    shape.radius = 0.62
    shape.height = 0.34
    cs.shape = shape
    cs.rotation_degrees.z = 90.0
    wheel.add_child(cs)
    add_child(wheel)

    if is_instance_valid(parent_body):
        var hinge: HingeJoint3D
        if powered_motor:
            var logic_motor := LogicMotorJoint.new()
            logic_motor.signal_enabled = motor_enabled
            hinge = logic_motor
            motors.append(logic_motor)
        else:
            hinge = HingeJoint3D.new()
        hinge.name = "MotorHinge" if powered_motor else "WheelHinge"
        hinge.global_position = wheel.global_position
        hinge.rotation_degrees.y = 90.0
        add_child(hinge)
        hinge.node_a = hinge.get_path_to(parent_body)
        hinge.node_b = hinge.get_path_to(wheel)
        _message("%s attached to %s" % ["Logic motor wheel" if powered_motor else "Wheel", parent_body.name])
    else:
        _message("Wheel placed free. Put it close to a part to auto-hinge.")
    return wheel

func _anchor_nearest() -> void:
    var pos := _build_position()
    var body := _nearest_part(pos, 4.0)
    if not is_instance_valid(body):
        _message("No assembly part close enough to anchor")
        return
    var joint := PinJoint3D.new()
    joint.name = "WorldAnchor_%s" % body.name
    joint.global_position = pos
    add_child(joint)
    joint.node_a = joint.get_path_to(body)
    joint.node_b = NodePath("")
    body.set_meta("world_anchored", true)
    _message("Anchored %s to the world" % body.name)

func _nearest_part(pos: Vector3, max_distance: float) -> RigidBody3D:
    var best: RigidBody3D
    var best_distance := max_distance
    for node in get_tree().get_nodes_in_group("assembly_parts"):
        if node is RigidBody3D:
            var d := node.global_position.distance_to(pos)
            if d < best_distance:
                best_distance = d
                best = node
    return best

func _save_blueprint() -> void:
    var parts: Array[Node] = get_tree().get_nodes_in_group("assembly_parts")
    if parts.is_empty():
        _message("No assembly parts to save")
        return

    var origin := Vector3.ZERO
    for node in parts:
        if node is RigidBody3D:
            origin += node.global_position
    origin /= float(parts.size())

    var records: Array = []
    for node in parts:
        if not node is RigidBody3D:
            continue
        var body := node as RigidBody3D
        var p := body.global_position - origin
        var r := body.global_rotation
        records.append({
            "name": body.name,
            "type": str(body.get_meta("part_type", "Platform")),
            "offset": [p.x,p.y,p.z],
            "rotation": [r.x,r.y,r.z],
            "parent": str(body.get_meta("parent_part_name", "")),
            "anchored": bool(body.get_meta("world_anchored", false))
        })

    var file := FileAccess.open(BLUEPRINT_PATH, FileAccess.WRITE)
    if file == null:
        _message("Could not write blueprint file")
        return
    file.store_string(JSON.stringify({"version":1,"parts":records}, "  "))
    file.close()
    _message("Blueprint saved: %d parts" % records.size())

func _deploy_blueprint() -> void:
    if not FileAccess.file_exists(BLUEPRINT_PATH):
        _message("No saved blueprint yet")
        return
    var file := FileAccess.open(BLUEPRINT_PATH, FileAccess.READ)
    if file == null:
        _message("Could not read blueprint")
        return
    var parsed = JSON.parse_string(file.get_as_text())
    file.close()
    if not parsed is Dictionary or not parsed.has("parts"):
        _message("Blueprint file is invalid")
        return
    var records: Array = parsed["parts"]
    var scrap_cost := 0
    var motor_cost := 0
    for record in records:
        var type_name := str(record.get("type", "Platform"))
        match type_name:
            "Platform": scrap_cost += 2
            "Beam": scrap_cost += 1
            "Wheel": scrap_cost += 2
            "MotorWheel":
                scrap_cost += 3
                motor_cost += 1
    if player.scrap < scrap_cost or int(player.components.get("motor",0)) < motor_cost:
        _message("Blueprint needs %d scrap + %d motors" % [scrap_cost,motor_cost])
        return
    player.scrap -= scrap_cost
    player.components["motor"] = int(player.components.get("motor",0)) - motor_cost

    var base := _build_position()
    var created := {}
    for record in records:
        var type_name := str(record.get("type", "Platform"))
        if type_name == "Wheel" or type_name == "MotorWheel":
            continue
        var offset_array: Array = record["offset"]
        var rot_array: Array = record["rotation"]
        var pos := base + Vector3(float(offset_array[0]),float(offset_array[1]),float(offset_array[2]))
        var rot := Vector3(float(rot_array[0]),float(rot_array[1]),float(rot_array[2]))
        var body: RigidBody3D
        if type_name == "Beam":
            body = _spawn_box_body("Beam", Vector3(0.32,0.32,3.0), 5.0, pos, rot)
        else:
            body = _spawn_box_body("Platform", Vector3(2.6,0.28,2.2), 18.0, pos, rot)
        created[str(record.get("name",""))] = body

    for record in records:
        var type_name := str(record.get("type", ""))
        if type_name != "Wheel" and type_name != "MotorWheel":
            continue
        var offset_array: Array = record["offset"]
        var rot_array: Array = record["rotation"]
        var pos := base + Vector3(float(offset_array[0]),float(offset_array[1]),float(offset_array[2]))
        var rot := Vector3(float(rot_array[0]),float(rot_array[1]),float(rot_array[2]))
        var parent_name := str(record.get("parent", ""))
        var parent_body: RigidBody3D = created.get(parent_name, null)
        _spawn_wheel_body(type_name == "MotorWheel", pos, rot, parent_body)

    _message("Blueprint reconstructed: %d parts" % records.size())

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "FAS PROTOTYPE\n%s\n1 platform  2 beam  3 wheel  4 logic motor\n5 anchor  X motors  O save blueprint  P rebuild" % text
