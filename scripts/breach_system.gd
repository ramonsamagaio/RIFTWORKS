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
    if event.keycode == KEY_0:
        _place_repulsion_emitter()

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    var bg := ColorRect.new()
    bg.position = Vector2(875, 364)
    bg.size = Vector2(390, 60)
    bg.color = Color(0.012,0.016,0.024,0.66)
    layer.add_child(bg)
    ui = Label.new()
    ui.position = Vector2(890, 375)
    ui.size = Vector2(360, 40)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("c6addf"))
    ui.text = "BREACH ENGINEERING\n0 repulsion emitter  |  requires a Breach Core"
    layer.add_child(ui)

func _spawn_deep_cores() -> void:
    var positions := [
        Vector3(-40,-16.7,-94),
        Vector3(-58,-17.6,-136),
        Vector3(-34,-17.6,-160),
        Vector3(-82,-17.6,-184),
        Vector3(-46,-17.6,-208)
    ]
    for pos in positions:
        var core := SalvageProp.new()
        core.configure("breach_core", "Repulsion Breach Core", 1, 0, Color("9874db"))
        core.position = pos + Vector3(rng.randf_range(-2.0,2.0),0.55,rng.randf_range(-2.0,2.0))
        add_child(core)

func _build_position() -> Vector3:
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 8.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit := player.get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        return hit.position + hit.normal * 0.06
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0,0.1,-3.0)

func _place_repulsion_emitter() -> void:
    if int(player.components.get("breach_core",0)) < 1 or int(player.components.get("electronics",0)) < 1 or player.scrap < 5:
        _message("Need 1 Breach Core + 1 electronics + 5 scrap")
        return
    player.components["breach_core"] = int(player.components.get("breach_core",0)) - 1
    player.components["electronics"] = int(player.components.get("electronics",0)) - 1
    player.scrap -= 5
    var emitter := RepulsionEmitter.new()
    emitter.position = _build_position()
    add_child(emitter)
    _message("Repulsion emitter online. Put physics parts near it, or link a logic source to it.")

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = "BREACH ENGINEERING\n%s" % text
