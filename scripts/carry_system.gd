class_name CarrySystem
extends Node3D

var player: RiftPlayer
var carried: SalvageProp
var ui: Label

func _ready() -> void:
    add_to_group("carry_system")
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _make_ui()

func _process(_delta: float) -> void:
    if not is_instance_valid(player) or not is_instance_valid(carried):
        return
    var forward := -player.camera.global_basis.z
    var target := player.global_position + Vector3(0,1.15,0) + Vector3(forward.x,0,forward.z).normalized() * 1.35
    carried.global_position = carried.global_position.lerp(target, 0.35)
    carried.global_rotation.y = player.yaw

func _unhandled_input(event: InputEvent) -> void:
    if not event is InputEventKey or not event.pressed or event.echo:
        return
    if event.keycode == KEY_Q and is_instance_valid(carried):
        drop_heavy()
    elif event.keycode == KEY_H and is_instance_valid(carried):
        deposit_heavy()

func pickup_heavy(prop: SalvageProp) -> bool:
    if not is_instance_valid(player) or not is_instance_valid(prop) or is_instance_valid(carried):
        return false
    carried = prop
    for child in carried.get_children():
        if child is CollisionShape3D:
            child.disabled = true
    player.set_meta("carrying_heavy", true)
    _message("CARRYING %s  |  Q drop  H secure at base" % carried.display_name)
    return true

func drop_heavy() -> void:
    if not is_instance_valid(carried): return
    carried.global_position = player.global_position + Basis(Vector3.UP,player.yaw) * Vector3(0,0.55,-1.8)
    for child in carried.get_children():
        if child is CollisionShape3D:
            child.set_deferred("disabled", false)
    carried = null
    player.set_meta("carrying_heavy", false)
    _message("Heavy component dropped")

func deposit_heavy() -> void:
    if not is_instance_valid(carried) or not _near_safe_base():
        _message("Heavy components can only be secured at your base/shelter")
        return
    player.add_component(carried.item_id, carried.amount)
    var name := carried.display_name
    carried.queue_free()
    carried = null
    player.set_meta("carrying_heavy", false)
    _message("%s secured into base inventory" % name)

func _near_safe_base() -> bool:
    if player.global_position.distance_to(Vector3(0,0,38)) <= 13.0:
        return true
    for node in get_tree().get_nodes_in_group("base_beacon"):
        if node is Node3D and player.global_position.distance_to(node.global_position) <= 11.0:
            return true
    return false

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    ui = Label.new()
    ui.position = Vector2(18,590)
    ui.size = Vector2(600,28)
    ui.add_theme_font_size_override("font_size",13)
    ui.add_theme_color_override("font_color",Color("a8b1b5"))
    ui.text = "Heavy salvage must be physically carried home."
    layer.add_child(ui)

func _message(text: String) -> void:
    if is_instance_valid(ui): ui.text = text
