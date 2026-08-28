class_name HuntingSystem
extends Node3D

var player: RiftPlayer

func _ready() -> void:
    await get_tree().process_frame
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    if event.keycode == KEY_F7:
        _place_harpoon()

func _build_transform() -> Transform3D:
    var from := player.camera.global_position
    var forward := -player.camera.global_basis.z
    var to := from + forward * 8.0
    var query := PhysicsRayQueryParameters3D.create(from,to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    var position: Vector3
    if hit.is_empty():
        position = player.global_position + Basis(Vector3.UP,player.yaw) * Vector3(0,0.1,-3.0)
    else:
        position = hit.get("position",player.global_position) as Vector3
        position += (hit.get("normal",Vector3.UP) as Vector3) * 0.05
    var flat_forward := Vector3(forward.x,0,forward.z).normalized()
    if flat_forward.length_squared() < 0.01:
        flat_forward = Vector3.FORWARD
    var basis := Basis.looking_at(flat_forward,Vector3.UP)
    return Transform3D(basis,position)

func _place_harpoon() -> void:
    if player.scrap < 8 or int(player.components.get("cable",0)) < 1 or int(player.components.get("electronics",0)) < 1:
        return
    player.scrap -= 8
    player.components["cable"] = int(player.components.get("cable",0)) - 1
    player.components["electronics"] = int(player.components.get("electronics",0)) - 1
    var harpoon := HarpoonDevice.new()
    harpoon.global_transform = _build_transform()
    add_child(harpoon)
