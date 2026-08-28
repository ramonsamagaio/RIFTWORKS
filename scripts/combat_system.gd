class_name CombatSystem
extends Node

var player: RiftPlayer
var cooldown := 0.0
var feedback: Label

func _ready() -> void:
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _make_feedback()

func _process(delta: float) -> void:
    cooldown = maxf(0.0, cooldown - delta)

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or cooldown > 0.0 or Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
        return
    if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
        _melee_strike()

func _melee_strike() -> void:
    cooldown = 0.42
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 3.7
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit := player.get_world_3d().direct_space_state.intersect_ray(query)
    if hit.is_empty():
        _flash_feedback("SWING", Color("8e989f"))
        return
    var collider = hit.collider
    if collider and collider.has_method("take_damage"):
        collider.take_damage(24.0)
        _flash_feedback("HIT", Color("e6c7a0"))
    else:
        _flash_feedback("CLANG", Color("9aa8b0"))

func _make_feedback() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 4
    add_child(layer)
    feedback = Label.new()
    feedback.position = Vector2(600, 388)
    feedback.size = Vector2(90, 30)
    feedback.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    feedback.add_theme_font_size_override("font_size", 13)
    feedback.modulate.a = 0.0
    layer.add_child(feedback)

func _flash_feedback(text: String, color: Color) -> void:
    feedback.text = text
    feedback.add_theme_color_override("font_color", color)
    feedback.modulate.a = 1.0
    var tween := create_tween()
    tween.tween_property(feedback, "modulate:a", 0.0, 0.24)
