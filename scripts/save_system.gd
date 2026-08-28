class_name RiftSaveSystem
extends Node

const SAVE_PATH := "user://riftworks_save.json"
const SAVE_VERSION := 1

var player: RiftPlayer
var base_system: BaseSystem
var feedback: Label

func _ready() -> void:
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    base_system = get_parent().get_node_or_null("BaseSystem") as BaseSystem
    _make_feedback()

func _unhandled_input(event: InputEvent) -> void:
    if not event is InputEventKey or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_F5:
            save_game()
        KEY_F9:
            load_game()

func _v3(value: Vector3) -> Array:
    return [value.x,value.y,value.z]

func _from_v3(value: Array) -> Vector3:
    if value.size() < 3:
        return Vector3.ZERO
    return Vector3(float(value[0]),float(value[1]),float(value[2]))

func save_game() -> void:
    if not is_instance_valid(player):
        return
    var player_data := {
        "position": _v3(player.global_position),
        "yaw": player.yaw,
        "pitch": player.pitch,
        "health": player.health,
        "flashlight_battery": player.flashlight_battery,
        "flashlight_on": player.flashlight_on,
        "scrap": player.scrap,
        "components": player.components.duplicate(true)
    }
    var base_data = null
    if is_instance_valid(base_system) and is_instance_valid(base_system.current_beacon):
        base_data = {"position": _v3(base_system.current_beacon.global_position)}

    var data := {
        "version": SAVE_VERSION,
        "world_seed": 731942,
        "player": player_data,
        "base": base_data
    }
    var file := FileAccess.open(SAVE_PATH, FileAccess.WRITE)
    if file == null:
        _message("SAVE FAILED")
        return
    file.store_string(JSON.stringify(data, "  "))
    file.close()
    _message("SAVED  |  player + inventory + claimed base")

func load_game() -> void:
    if not is_instance_valid(player) or not FileAccess.file_exists(SAVE_PATH):
        _message("NO SAVE FOUND")
        return
    var file := FileAccess.open(SAVE_PATH, FileAccess.READ)
    if file == null:
        _message("LOAD FAILED")
        return
    var parsed = JSON.parse_string(file.get_as_text())
    file.close()
    if not parsed is Dictionary or not parsed.has("player"):
        _message("SAVE IS INVALID")
        return
    var p: Dictionary = parsed["player"]
    player.global_position = _from_v3(p.get("position", [0,0,44]))
    player.velocity = Vector3.ZERO
    player.yaw = float(p.get("yaw",0.0))
    player.pitch = float(p.get("pitch",-0.16))
    player.health = float(p.get("health",100.0))
    player.flashlight_battery = float(p.get("flashlight_battery",100.0))
    player.scrap = int(p.get("scrap",0))
    var loaded_components = p.get("components", {})
    if loaded_components is Dictionary:
        player.components = loaded_components.duplicate(true)
    player.set_flashlight(bool(p.get("flashlight_on",true)))

    var base_data = parsed.get("base", null)
    if base_data is Dictionary and base_data.has("position") and is_instance_valid(base_system):
        base_system.restore_base(_from_v3(base_data["position"]))
    _message("LOADED")

func _make_feedback() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 5
    add_child(layer)
    feedback = Label.new()
    feedback.position = Vector2(18, 650)
    feedback.size = Vector2(440, 26)
    feedback.add_theme_font_size_override("font_size", 13)
    feedback.add_theme_color_override("font_color", Color("8ca0ad"))
    feedback.text = "F5 save  |  F9 load"
    layer.add_child(feedback)

func _message(text: String) -> void:
    if not is_instance_valid(feedback): return
    feedback.text = text
    var tween := create_tween()
    tween.tween_interval(2.2)
    tween.tween_callback(func():
        if is_instance_valid(feedback): feedback.text = "F5 save  |  F9 load"
    )
