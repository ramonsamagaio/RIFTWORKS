class_name BreachSystem
extends Node3D

var player: RiftPlayer
var ui: Label
var rng := RandomNumberGenerator.new()

func _ready() -> void:
    rng.seed = 918244
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _spawn_deep_cores()
    _make_ui()

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_0:
            _place_repulsion_emitter()
        KEY_N:
            _place_attraction_emitter()
        KEY_M:
            _place_luminance_emitter()

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    var bg := ColorRect.new()
    bg.position = Vector2(875, 364)
    bg.size = Vector2(390, 76)
    bg.color = Color(0.012,0.016,0.024,0.66)
    layer.add_child(bg)
    ui = Label.new()
    ui.position = Vector2(890, 375)
    ui.size = Vector2(360, 56)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("c6addf"))
    ui.text = "BREACH ENGINEERING\n0 repulsion   N attraction   M luminance"
    layer.add_child(ui)

func _spawn_deep_cores() -> void:
    var positions: Array[Vector3] = [
        Vector3(-40,-16.7,-94),
        Vector3(-58,-17.6,-136),
        Vector3(-34,-17.6,-160),
        Vector3(-82,-17.6,-184),
        Vector3(-46,-17.6,-208)
    ]
    var names: Array[String] = [
        "Repulsion Breach Core",
        "Attraction Breach Core",
        "Luminance Breach Core",
        "Dense Breach Core",
        "Resonant Breach Core"
    ]
    var colors: Array[Color] = [
        Color("9874db"), Color("55c0c8"), Color("ffd99a"), Color("8b79bd"), Color("b272c2")
    ]
    for i in range(positions.size()):
        var core := SalvageProp.new()
        core.configure("breach_core", names[i], 1, 0, colors[i])
        core.position = positions[i] + Vector3(rng.randf_range(-2.0,2.0),0.55,rng.randf_range(-2.0,2.0))
        add_child(core)

func _build_position() -> Vector3:
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 8.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        var hit_position: Vector3 = hit.get("position", player.global_position)
        var hit_normal: Vector3 = hit.get("normal", Vector3.UP)
        return hit_position + hit_normal * 0.06
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0,0.1,-3.0)

func _can_build() -> bool:
    return int(player.components.get("breach_core",0)) >= 1 and int(player.components.get("electronics",0)) >= 1 and player.scrap >= 5

func _pay_build() -> void:
    player.components["breach_core"] = int(player.components.get("breach_core",0)) - 1
    player.components["electronics"] = int(player.components.get("electronics",0)) - 1
    player.scrap -= 5

func _place_repulsion_emitter() -> void:
    if not _can_build():
        _message("Need 1 Breach Core + 1 electronics + 5 scrap")
        return
    _pay_build()
    var emitter := RepulsionEmitter.new()
    emitter.position = _build_position()
    add_child(emitter)
    _message("REPULSION ONLINE. Pushes rigid bodies and accepts logic signals.")

func _place_attraction_emitter() -> void:
    if not _can_build():
        _message("Need 1 Breach Core + 1 electronics + 5 scrap")
        return
    _pay_build()
    var emitter := AttractionEmitter.new()
    emitter.position = _build_position()
    add_child(emitter)
    _message("ATTRACTION ONLINE. Pulls rigid bodies and accepts logic signals.")

func _place_luminance_emitter() -> void:
    if not _can_build():
        _message("Need 1 Breach Core + 1 electronics + 5 scrap")
        return
    _pay_build()
    var emitter := LuminanceEmitter.new()
    emitter.position = _build_position()
    add_child(emitter)
    _message("LUMINANCE ONLINE. High-output Breach light accepts logic signals.")

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "BREACH ENGINEERING\n%s" % text
