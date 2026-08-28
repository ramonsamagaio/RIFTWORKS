class_name LogicSystem
extends Node3D

var player: RiftPlayer
var ui: Label
var connection_count := 0

func _ready() -> void:
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _make_ui()

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_6:
            _place_source(LogicSource.SourceKind.BUTTON)
        KEY_7:
            _place_source(LogicSource.SourceKind.PROXIMITY_SENSOR)
        KEY_8:
            _place_receiver(LogicReceiver.ReceiverKind.SIGNAL_LAMP)
        KEY_9:
            _place_receiver(LogicReceiver.ReceiverKind.ALARM_BEACON)
        KEY_K:
            _connect_nearest_pair()

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    var bg := ColorRect.new()
    bg.position = Vector2(875, 432)
    bg.size = Vector2(390, 126)
    bg.color = Color(0.012,0.016,0.024,0.66)
    layer.add_child(bg)
    ui = Label.new()
    ui.position = Vector2(890, 443)
    ui.size = Vector2(360, 100)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("b9c8d2"))
    ui.text = "LOGIC PROTOTYPE\n6 button  7 proximity sensor\n8 signal lamp  9 alarm beacon\nK connect nearest source -> receiver"
    layer.add_child(ui)

func _build_position() -> Vector3:
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 8.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit := player.get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        return hit.position + hit.normal * 0.08
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0, 0.1, -3.0)

func _can_pay(scrap_cost: int, electronics_cost: int, cable_cost: int) -> bool:
    return player.scrap >= scrap_cost and int(player.components.get("electronics",0)) >= electronics_cost and int(player.components.get("cable",0)) >= cable_cost

func _pay(scrap_cost: int, electronics_cost: int, cable_cost: int) -> void:
    player.scrap -= scrap_cost
    player.components["electronics"] = int(player.components.get("electronics",0)) - electronics_cost
    player.components["cable"] = int(player.components.get("cable",0)) - cable_cost

func _place_source(kind: LogicSource.SourceKind) -> void:
    if not _can_pay(2,1,0):
        _message("Need 2 scrap + 1 electronics")
        return
    _pay(2,1,0)
    var source := LogicSource.new()
    source.configure(kind)
    source.position = _build_position()
    add_child(source)
    _message("Placed %s" % ("button" if kind == LogicSource.SourceKind.BUTTON else "proximity sensor"))

func _place_receiver(kind: LogicReceiver.ReceiverKind) -> void:
    if not _can_pay(2,1,0):
        _message("Need 2 scrap + 1 electronics")
        return
    _pay(2,1,0)
    var receiver := LogicReceiver.new()
    receiver.configure(kind)
    receiver.position = _build_position()
    add_child(receiver)
    _message("Placed %s" % ("signal lamp" if kind == LogicReceiver.ReceiverKind.SIGNAL_LAMP else "alarm beacon"))

func _connect_nearest_pair() -> void:
    if int(player.components.get("cable",0)) < 1:
        _message("Need 1 cable coil")
        return
    var source := _nearest_source(player.global_position, 14.0)
    var receiver := _nearest_receiver(player.global_position, 14.0)
    if not is_instance_valid(source) or not is_instance_valid(receiver):
        _message("Stand near a source and receiver to link them")
        return
    if source.global_position.distance_to(receiver.global_position) > 18.0:
        _message("Source and receiver are too far apart")
        return
    if not receiver.has_method("set_signal"):
        _message("Selected receiver cannot accept a logic signal")
        return
    player.components["cable"] = int(player.components.get("cable",0)) - 1
    source.output_changed.connect(Callable(receiver, "set_signal"))
    receiver.call("set_signal", source.output)
    _make_signal_cable(source, receiver)
    connection_count += 1
    _message("Signal link #%d connected" % connection_count)

func _nearest_source(pos: Vector3, max_distance: float) -> LogicSource:
    var best: LogicSource
    var best_distance := max_distance
    for node in get_tree().get_nodes_in_group("logic_sources"):
        if node is LogicSource:
            var d := node.global_position.distance_to(pos)
            if d < best_distance:
                best_distance = d
                best = node
    return best

func _nearest_receiver(pos: Vector3, max_distance: float) -> Node3D:
    var best: Node3D
    var best_distance := max_distance
    for node in get_tree().get_nodes_in_group("logic_receivers"):
        if node is Node3D and node.has_method("set_signal"):
            var d := node.global_position.distance_to(pos)
            if d < best_distance:
                best_distance = d
                best = node
    return best

func _make_signal_cable(source: LogicSource, receiver: Node3D) -> void:
    var a := source.global_position + Vector3(0,0.45,0)
    var b := receiver.global_position + Vector3(0,0.45,0)
    var delta := b - a
    if delta.length() < 0.05: return
    var cable := MeshInstance3D.new()
    cable.position = (a+b)*0.5
    var mesh := CylinderMesh.new()
    mesh.top_radius = 0.018
    mesh.bottom_radius = 0.018
    mesh.height = delta.length()
    mesh.radial_segments = 5
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("263039")
    mat.metallic = 0.3
    mat.roughness = 0.78
    mesh.material = mat
    cable.mesh = mesh
    cable.quaternion = Quaternion(Vector3.UP, delta.normalized())
    add_child(cable)

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "LOGIC PROTOTYPE\n%s\n6 button  7 sensor  8 lamp  9 alarm\nK connect nearest source -> receiver" % text
