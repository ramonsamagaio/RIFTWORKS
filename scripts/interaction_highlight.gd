class_name InteractionHighlight
extends Node

var player: RiftPlayer
var highlighted_root: Node
var highlighted_meshes: Array[GeometryInstance3D] = []
var overlay: StandardMaterial3D

func _ready() -> void:
    overlay = StandardMaterial3D.new()
    overlay.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    overlay.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    overlay.albedo_color = Color(0.44, 0.72, 0.92, 0.10)
    overlay.emission_enabled = true
    overlay.emission = Color(0.25, 0.58, 0.82)
    overlay.emission_energy_multiplier = 0.28
    await get_tree().process_frame
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer

func _process(_delta: float) -> void:
    if not is_instance_valid(player) or not is_instance_valid(player.camera):
        return
    var target := _find_interactable()
    if target == highlighted_root:
        return
    _clear_highlight()
    if is_instance_valid(target):
        highlighted_root = target
        _apply_recursive(target)

func _find_interactable() -> Node:
    var from := player.camera.global_position
    var to := from + (-player.camera.global_basis.z) * 5.7
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    if hit.is_empty():
        return null
    var collider: Object = hit.get("collider")
    if collider is Node and (collider.has_method("interact") or collider.has_method("get_prompt_text")):
        return collider as Node
    return null

func _apply_recursive(node: Node) -> void:
    if node is GeometryInstance3D:
        var geometry := node as GeometryInstance3D
        geometry.material_overlay = overlay
        highlighted_meshes.append(geometry)
    for child in node.get_children():
        _apply_recursive(child)

func _clear_highlight() -> void:
    for mesh in highlighted_meshes:
        if is_instance_valid(mesh) and mesh.material_overlay == overlay:
            mesh.material_overlay = null
    highlighted_meshes.clear()
    highlighted_root = null
