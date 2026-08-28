class_name AssemblySystem
extends Node3D

const BLUEPRINT_PATH := "user://riftworks_last_blueprint.json"
const GRID_STEP := 0.5

var player: RiftPlayer
var motors: Array[LogicMotorJoint] = []
var pistons: Array[LogicPistonJoint] = []
var motor_enabled: bool = true
var snap_enabled: bool = true
var ui: Label
var part_counter: int = 0

func _ready() -> void:
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _make_ui()

func _process(_delta: float) -> void:
    for child: Node in get_children():
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
        KEY_L:
            snap_enabled = not snap_enabled
            _message("Magnetic snap %s" % ("ON" if snap_enabled else "OFF"))
        KEY_Y:
            _weld_nearest_pair()
        KEY_I:
            _create_piston_nearest_pair()
        KEY_X:
            motor_enabled = not motor_enabled
            for motor: LogicMotorJoint in motors:
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
    bg.size = Vector2(390, 152)
    bg.color = Color(0.012,0.016,0.024,0.66)
    layer.add_child(bg)
    ui = Label.new()
    ui.position = Vector2(890, 578)
    ui.size = Vector2(360, 132)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("b9c8d2"))
    ui.text = "FAS PROTOTYPE\n1 platform  2 beam  3 wheel  4 logic motor wheel\n5 anchor  L snap  Y weld  I logic piston\nX motors  O save blueprint  P rebuild"
    layer.add_child(ui)

func _raw_build_position() -> Vector3:
    var camera: Camera3D = player.camera
    var from: Vector3 = camera.global_position
    var to: Vector3 = from + (-camera.global_basis.z) * 8.0
    var query: PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        var hit_position: Vector3 = hit.get("position", player.global_position)
        var hit_normal: Vector3 = hit.get("normal", Vector3.UP)
        return hit_position + hit_normal * 0.35
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0, 0.7, -3.5)

func _build_position(part_size: Vector3 = Vector3.ZERO) -> Vector3:
    var raw := _raw_build_position()
    if not snap_enabled:
        return raw
    return _snap_build_position(raw, part_size)

func _snap_build_position(raw: Vector3, part_size: Vector3) -> Vector3:
    var grid_snapped := Vector3(
        snappedf(raw.x, GRID_STEP),
        snappedf(raw.y, GRID_STEP),
        snappedf(raw.z, GRID_STEP)
    )
    if part_size == Vector3.ZERO:
        return grid_snapped

    var target := _nearest_part(raw, 4.0)
    if not is_instance_valid(target):
        return grid_snapped

    var target_size: Vector3 = target.get_meta("part_size", Vector3.ONE)
    var local_delta := target.global_basis.inverse() * (raw - target.global_position)
    var ax := absf(local_delta.x)
    var ay := absf(local_delta.y)
    var az := absf(local_delta.z)
    var local_snap := Vector3.ZERO

    if ax >= ay and ax >= az:
        var sign_x := 1.0 if local_delta.x >= 0.0 else -1.0
        local_snap.x = sign_x * (target_size.x + part_size.x) * 0.5
        local_snap.y = snappedf(local_delta.y, GRID_STEP)
        local_snap.z = snappedf(local_delta.z, GRID_STEP)
    elif ay >= ax and ay >= az:
        var sign_y := 1.0 if local_delta.y >= 0.0 else -1.0
        local_snap.y = sign_y * (target_size.y + part_size.y) * 0.5
        local_snap.x = snappedf(local_delta.x, GRID_STEP)
        local_snap.z = snappedf(local_delta.z, GRID_STEP)
    else:
        var sign_z := 1.0 if local_delta.z >= 0.0 else -1.0
        local_snap.z = sign_z * (target_size.z + part_size.z) * 0.5
        local_snap.x = snappedf(local_delta.x, GRID_STEP)
        local_snap.y = snappedf(local_delta.y, GRID_STEP)

    return target.global_position + target.global_basis * local_snap

func _material(color: Color, metallic: float = 0.55, roughness: float = 0.58) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _place_box_part(label: String, size: Vector3, mass_value: float, scrap_cost: int) -> void:
    if player.scrap < scrap_cost:
        _message("Need %d scrap" % scrap_cost)
        return
    player.scrap -= scrap_cost
    _spawn_box_body(label, size, mass_value, _build_position(size), Vector3.ZERO)
    _message("Placed %s. Snap %s." % [label, "ON" if snap_enabled else "OFF"])

func _spawn_box_body(label: String, size: Vector3, mass_value: float, pos: Vector3, rotation: Vector3) -> RigidBody3D:
    var body := RigidBody3D.new()
    part_counter += 1
    body.name = "%s_%03d" % [label, part_counter]
    body.mass = mass_value
    body.global_position = pos
    body.global_rotation = rotation
    body.continuous_cd = true
    body.add_to_group("assembly_parts")
    body.set_meta("part_type", label)
    body.set_meta("part_size", size)

    var mesh_instance := MeshInstance3D.new()
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _material(Color("58636a") if label == "Platform" else Color("475158"))
    mesh_instance.mesh = mesh
    mesh_instance.gi_mode = GeometryInstance3D.GI_MODE_DYNAMIC
    body.add_child(mesh_instance)

    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = size
    cs.shape = shape
    body.add_child(cs)
    add_child(body)
    return body

func _place_wheel(powered_motor: bool) -> void:
    var scrap_cost: int = 3 if powered_motor else 2
    if player.scrap < scrap_cost:
        _message("Need %d scrap" % scrap_cost)
        return
    if powered_motor and int(player.components.get("motor", 0)) < 1:
        _message("Need an industrial motor")
        return
    player.scrap -= scrap_cost
    if powered_motor:
        player.components["motor"] = int(player.components.get("motor", 0)) - 1

    var pos: Vector3 = _build_position(Vector3(0.34,1.24,1.24))
    var parent_body: RigidBody3D = _nearest_part(pos, 3.6)
    _spawn_wheel_body(powered_motor, pos, Vector3.ZERO, parent_body)

func _spawn_wheel_body(powered_motor: bool, pos: Vector3, rotation: Vector3, parent_body: RigidBody3D = null) -> RigidBody3D:
    var wheel := RigidBody3D.new()
    part_counter += 1
    var type_name: String = "MotorWheel" if powered_motor else "Wheel"
    wheel.name = type_name + "_%03d" % part_counter
    wheel.mass = 4.5
    wheel.global_position = pos
    wheel.global_rotation = rotation
    wheel.continuous_cd = true
    wheel.add_to_group("assembly_parts")
    wheel.set_meta("part_type", type_name)
    wheel.set_meta("part_size", Vector3(0.34,1.24,1.24))
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
    mesh_instance.gi_mode = GeometryInstance3D.GI_MODE_DYNAMIC
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
    var pos: Vector3 = _raw_build_position()
    var body: RigidBody3D = _nearest_part(pos, 4.0)
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
    var best_distance: float = max_distance
    for raw_node: Node in get_tree().get_nodes_in_group("assembly_parts"):
        var body: RigidBody3D = raw_node as RigidBody3D
        if not is_instance_valid(body):
            continue
        var distance: float = body.global_position.distance_to(pos)
        if distance < best_distance:
            best_distance = distance
            best = body
    return best

func _nearest_two_parts(pos: Vector3, max_distance: float) -> Array[RigidBody3D]:
    var candidates: Array[RigidBody3D] = []
    for raw_node: Node in get_tree().get_nodes_in_group("assembly_parts"):
        var body := raw_node as RigidBody3D
        if is_instance_valid(body) and body.global_position.distance_to(pos) <= max_distance:
            candidates.append(body)
    candidates.sort_custom(func(a: RigidBody3D, b: RigidBody3D) -> bool:
        return a.global_position.distance_squared_to(pos) < b.global_position.distance_squared_to(pos)
    )
    if candidates.size() > 2:
        candidates.resize(2)
    return candidates

func _lock_axis(joint: Generic6DOFJoint3D, axis: int) -> void:
    match axis:
        0:
            joint.set_flag_x(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
            joint.set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, 0.0)
            joint.set_param_x(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, 0.0)
            joint.set_flag_x(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
            joint.set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
            joint.set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)
        1:
            joint.set_flag_y(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
            joint.set_param_y(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, 0.0)
            joint.set_param_y(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, 0.0)
            joint.set_flag_y(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
            joint.set_param_y(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
            joint.set_param_y(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)
        2:
            joint.set_flag_z(Generic6DOFJoint3D.FLAG_ENABLE_LINEAR_LIMIT, true)
            joint.set_param_z(Generic6DOFJoint3D.PARAM_LINEAR_LOWER_LIMIT, 0.0)
            joint.set_param_z(Generic6DOFJoint3D.PARAM_LINEAR_UPPER_LIMIT, 0.0)
            joint.set_flag_z(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, true)
            joint.set_param_z(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, 0.0)
            joint.set_param_z(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.0)

func _weld_nearest_pair() -> void:
    if player.scrap < 1:
        _message("Need 1 scrap for a weld")
        return
    var parts := _nearest_two_parts(_raw_build_position(), 5.0)
    if parts.size() < 2:
        _message("Aim between two nearby assembly parts to weld them")
        return
    player.scrap -= 1
    var a := parts[0]
    var b := parts[1]
    var joint := Generic6DOFJoint3D.new()
    joint.name = "Weld_%s_%s" % [a.name,b.name]
    joint.global_position = (a.global_position + b.global_position) * 0.5
    for axis in range(3):
        _lock_axis(joint, axis)
    add_child(joint)
    joint.node_a = joint.get_path_to(a)
    joint.node_b = joint.get_path_to(b)
    b.set_meta("parent_part_name", a.name)
    b.set_meta("connection_type", "weld")
    _message("WELDED %s + %s" % [a.name,b.name])

func _create_piston_nearest_pair() -> void:
    if player.scrap < 3 or int(player.components.get("motor",0)) < 1:
        _message("Need 3 scrap + 1 industrial motor for a piston")
        return
    var parts := _nearest_two_parts(_raw_build_position(), 6.0)
    if parts.size() < 2:
        _message("Aim between two nearby assembly parts to install a piston")
        return
    player.scrap -= 3
    player.components["motor"] = int(player.components.get("motor",0)) - 1
    var a := parts[0]
    var b := parts[1]
    var axis := b.global_position - a.global_position
    if axis.length_squared() < 0.01:
        axis = Vector3.RIGHT
    else:
        axis = axis.normalized()

    var piston := LogicPistonJoint.new()
    piston.name = "LogicPiston_%s_%s" % [a.name,b.name]
    piston.configure(3.4, 1.25, 620.0)
    piston.global_position = (a.global_position + b.global_position) * 0.5
    piston.quaternion = Quaternion(Vector3.RIGHT, axis)
    add_child(piston)
    piston.node_a = piston.get_path_to(a)
    piston.node_b = piston.get_path_to(b)
    pistons.append(piston)
    b.set_meta("parent_part_name", a.name)
    b.set_meta("connection_type", "piston")
    _message("LOGIC PISTON installed. E toggles; button/sensor can drive it.")

func _save_blueprint() -> void:
    var parts: Array[Node] = get_tree().get_nodes_in_group("assembly_parts")
    if parts.is_empty():
        _message("No assembly parts to save")
        return

    var origin := Vector3.ZERO
    var rigid_part_count: int = 0
    for node: Node in parts:
        var body: RigidBody3D = node as RigidBody3D
        if is_instance_valid(body):
            origin += body.global_position
            rigid_part_count += 1
    if rigid_part_count == 0:
        _message("No valid rigid assembly parts to save")
        return
    origin /= float(rigid_part_count)

    var records: Array = []
    for node: Node in parts:
        var body: RigidBody3D = node as RigidBody3D
        if not is_instance_valid(body):
            continue
        var p: Vector3 = body.global_position - origin
        var r: Vector3 = body.global_rotation
        records.append({
            "name": body.name,
            "type": str(body.get_meta("part_type", "Platform")),
            "offset": [p.x,p.y,p.z],
            "rotation": [r.x,r.y,r.z],
            "parent": str(body.get_meta("parent_part_name", "")),
            "connection": str(body.get_meta("connection_type", "")),
            "anchored": bool(body.get_meta("world_anchored", false))
        })

    var file: FileAccess = FileAccess.open(BLUEPRINT_PATH, FileAccess.WRITE)
    if file == null:
        _message("Could not write blueprint file")
        return
    file.store_string(JSON.stringify({"version":2,"parts":records}, "  "))
    file.close()
    _message("Blueprint saved: %d parts" % records.size())

func _deploy_blueprint() -> void:
    if not FileAccess.file_exists(BLUEPRINT_PATH):
        _message("No saved blueprint yet")
        return
    var file: FileAccess = FileAccess.open(BLUEPRINT_PATH, FileAccess.READ)
    if file == null:
        _message("Could not read blueprint")
        return
    var parsed: Variant = JSON.parse_string(file.get_as_text())
    file.close()
    if not parsed is Dictionary or not parsed.has("parts"):
        _message("Blueprint file is invalid")
        return
    var records: Array = parsed["parts"] as Array
    var scrap_cost: int = 0
    var motor_cost: int = 0
    for record_value: Variant in records:
        var record: Dictionary = record_value as Dictionary
        var type_name: String = str(record.get("type", "Platform"))
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

    var base: Vector3 = _build_position()
    var created: Dictionary = {}
    for record_value: Variant in records:
        var record: Dictionary = record_value as Dictionary
        var type_name: String = str(record.get("type", "Platform"))
        if type_name == "Wheel" or type_name == "MotorWheel":
            continue
        var offset_array: Array = record["offset"] as Array
        var rot_array: Array = record["rotation"] as Array
        var pos: Vector3 = base + Vector3(float(offset_array[0]),float(offset_array[1]),float(offset_array[2]))
        var rot: Vector3 = Vector3(float(rot_array[0]),float(rot_array[1]),float(rot_array[2]))
        var body: RigidBody3D
        if type_name == "Beam":
            body = _spawn_box_body("Beam", Vector3(0.32,0.32,3.0), 5.0, pos, rot)
        else:
            body = _spawn_box_body("Platform", Vector3(2.6,0.28,2.2), 18.0, pos, rot)
        created[str(record.get("name",""))] = body

    for record_value: Variant in records:
        var record: Dictionary = record_value as Dictionary
        var type_name: String = str(record.get("type", ""))
        if type_name != "Wheel" and type_name != "MotorWheel":
            continue
        var offset_array: Array = record["offset"] as Array
        var rot_array: Array = record["rotation"] as Array
        var pos: Vector3 = base + Vector3(float(offset_array[0]),float(offset_array[1]),float(offset_array[2]))
        var rot: Vector3 = Vector3(float(rot_array[0]),float(rot_array[1]),float(rot_array[2]))
        var parent_name: String = str(record.get("parent", ""))
        var parent_body: RigidBody3D = created.get(parent_name, null) as RigidBody3D
        _spawn_wheel_body(type_name == "MotorWheel", pos, rot, parent_body)

    _message("Blueprint reconstructed: %d parts" % records.size())

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "FAS PROTOTYPE\n%s\n1 platform 2 beam 3 wheel 4 motor | L snap Y weld I piston\n5 anchor X motors O save P rebuild" % text
