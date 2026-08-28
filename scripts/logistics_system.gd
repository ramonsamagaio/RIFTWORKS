class_name LogisticsSystem
extends Node3D

var player: RiftPlayer

func _ready() -> void:
    await get_tree().process_frame
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    if event.keycode == KEY_U:
        _place_winch()

func _build_position() -> Vector3:
    var from := player.camera.global_position
    var to := from + (-player.camera.global_basis.z) * 8.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        var hit_position: Vector3 = hit.get("position", player.global_position)
        var hit_normal: Vector3 = hit.get("normal", Vector3.UP)
        return hit_position + hit_normal * 0.08
    return player.global_position + Basis(Vector3.UP, player.yaw) * Vector3(0,0.1,-3.0)

func _place_winch() -> void:
    if player.scrap < 6 or int(player.components.get("motor",0)) < 1 or int(player.components.get("cable",0)) < 1:
        return
    player.scrap -= 6
    player.components["motor"] = int(player.components.get("motor",0)) - 1
    player.components["cable"] = int(player.components.get("cable",0)) - 1
    var winch := WinchDevice.new()
    winch.position = _build_position()
    add_child(winch)
