class_name AssemblySystem
extends Node3D

var player: RiftPlayer
var motors: Array[HingeJoint3D] = []
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
                    motor.set_flag(HingeJoint3D.FLAG_ENABLE_MOTOR, motor_enabled)
            _message("Assembly motors %s" % ("ON" if motor_enabled else "OFF"))

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
    ui.text = "FAS PROTOTYPE\n1 platform  2 beam  3 wheel  4 motor wheel\n5 anchor nearest part  |  X motors ON/OFF\nParts use physics + generic joints, not recipes."
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
    var body := RigidBody3D.new()
    part_counter += 1
    body.name = "%s_%03d" % [label, part_counter]
    body.mass = mass
    body.position = _build_position()
    body.continuous_cd = true
    body.add_to_group("assembly_parts")

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
    _message("Placed %s. Physics is live." % label)

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
    var wheel := RigidBody3D.new()
    part_counter += 1
    wheel.name = ("MotorWheel" if powered_motor else "Wheel") + "_%03d" % part_counter
    wheel.mass = 4.5
    wheel.position = pos
    wheel.continuous_cd = true
    wheel.add_to_group("assembly_parts")

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
        var hinge := HingeJoint3D.new()
        hinge.name = "MotorHinge" if powered_motor else "WheelHinge"
        hinge.global_position = wheel.global_position
        hinge.rotation_degrees.y = 90.0
        add_child(hinge)
        hinge.node_a = hinge.get_path_to(parent_body)
        hinge.node_b = hinge.get_path_to(wheel)
        if powered_motor:
            hinge.set_flag(HingeJoint3D.FLAG_ENABLE_MOTOR, motor_enabled)
            hinge.set_param(HingeJoint3D.PARAM_MOTOR_TARGET_VELOCITY, 8.5)
            hinge.set_param(HingeJoint3D.PARAM_MOTOR_MAX_IMPULSE, 28.0)
            motors.append(hinge)
        _message("%s attached to %s" % ["Motor wheel" if powered_motor else "Wheel", parent_body.name])
    else:
        _message("Wheel placed free. Put it close to a part to auto-hinge.")

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

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "FAS PROTOTYPE\n%s\n1 platform  2 beam  3 wheel  4 motor wheel\n5 anchor  |  X motors %s" % [text, "ON" if motor_enabled else "OFF"]
