class_name BaseSystem
extends Node3D

var player: RiftPlayer
var current_beacon: StaticBody3D
var storage: Dictionary = {}
var ui: Label

func _ready() -> void:
    add_to_group("base_system")
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    if is_instance_valid(player) and not player.died.is_connected(_on_player_died):
        player.died.connect(_on_player_died)
    _make_ui()

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    if event.keycode != KEY_C:
        return
    if event.shift_pressed:
        _withdraw_storage()
    elif event.ctrl_pressed:
        _deposit_field_inventory()
    else:
        _claim_base()

func _make_ui() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 3
    add_child(layer)
    ui = Label.new()
    ui.position = Vector2(18, 620)
    ui.size = Vector2(720, 44)
    ui.add_theme_font_size_override("font_size", 14)
    ui.add_theme_color_override("font_color", Color("c5c9b1"))
    ui.text = "C claim base  |  CTRL+C store field components  |  SHIFT+C withdraw base components"
    layer.add_child(ui)

func _claim_base() -> void:
    if player.scrap < 6 or int(player.components.get("electronics",0)) < 1:
        _message("Need 6 scrap + 1 electronics to establish a base beacon")
        return
    player.scrap -= 6
    player.components["electronics"] = int(player.components.get("electronics",0)) - 1
    if is_instance_valid(current_beacon):
        current_beacon.queue_free()
    var pos := player.global_position
    current_beacon = _make_beacon(pos + Basis(Vector3.UP, player.yaw) * Vector3(1.8,0,-1.8))
    var respawn := current_beacon.global_position + Vector3(0,0.2,2.0)
    player.set_meta("respawn_position", respawn)
    player.set_meta("base_claimed", true)
    _message("BASE CLAIMED  |  local storage and respawn are online")

func restore_base(position: Vector3, restored_storage: Dictionary = {}) -> void:
    if is_instance_valid(current_beacon):
        current_beacon.queue_free()
    current_beacon = _make_beacon(position)
    storage = restored_storage.duplicate(true)
    if is_instance_valid(player):
        player.set_meta("respawn_position", position + Vector3(0,0.2,2.0))
        player.set_meta("base_claimed", true)

func _on_player_died() -> void:
    if not is_instance_valid(player) or not player.has_meta("respawn_position"):
        return
    player.global_position = player.get_meta("respawn_position") as Vector3
    player.velocity = Vector3.ZERO
    _message("Returned to claimed base. Built infrastructure remains yours.")

func _near_base(max_distance := 12.0) -> bool:
    return is_instance_valid(current_beacon) and player.global_position.distance_to(current_beacon.global_position) <= max_distance

func store_component(item_id: String, amount: int) -> void:
    if amount <= 0:
        return
    storage[item_id] = int(storage.get(item_id,0)) + amount

func take_component(item_id: String, amount: int) -> int:
    var available := int(storage.get(item_id,0))
    var taken := mini(maxi(amount,0), available)
    if taken <= 0:
        return 0
    storage[item_id] = available - taken
    return taken

func _deposit_field_inventory() -> void:
    if not _near_base():
        _message("Stand near your claimed base to store field components")
        return
    var moved := 0
    for key_value: Variant in player.components.keys():
        var key := str(key_value)
        var amount := int(player.components.get(key,0))
        if amount <= 0:
            continue
        store_component(key, amount)
        player.components[key] = 0
        moved += amount
    _message("Stored %d field components  |  %s" % [moved, get_storage_summary()])

func _withdraw_storage() -> void:
    if not _near_base():
        _message("Stand near your claimed base to withdraw components")
        return
    var moved := 0
    for key_value: Variant in storage.keys():
        var key := str(key_value)
        var amount := int(storage.get(key,0))
        if amount <= 0:
            continue
        player.components[key] = int(player.components.get(key,0)) + amount
        storage[key] = 0
        moved += amount
    _message("Withdrew %d components from base storage" % moved)

func get_storage_summary() -> String:
    var nonzero: Array[String] = []
    for key_value: Variant in storage.keys():
        var key := str(key_value)
        var amount := int(storage.get(key,0))
        if amount > 0:
            nonzero.append("%s:%d" % [key,amount])
    if nonzero.is_empty():
        return "BASE STORAGE EMPTY"
    nonzero.sort()
    return "BASE " + "  ".join(nonzero)

func _make_beacon(pos: Vector3) -> StaticBody3D:
    var body := StaticBody3D.new()
    body.name = "BaseBeacon"
    body.position = pos
    body.add_to_group("base_beacon")

    var cs := CollisionShape3D.new()
    var shape := CylinderShape3D.new()
    shape.radius = 0.55
    shape.height = 1.7
    cs.shape = shape
    cs.position.y = 0.85
    body.add_child(cs)

    var base_mesh := MeshInstance3D.new()
    base_mesh.position.y = 0.65
    var cylinder := CylinderMesh.new()
    cylinder.top_radius = 0.38
    cylinder.bottom_radius = 0.55
    cylinder.height = 1.3
    cylinder.radial_segments = 8
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("343a3d")
    mat.metallic = 0.58
    mat.roughness = 0.54
    cylinder.material = mat
    base_mesh.mesh = cylinder
    body.add_child(base_mesh)

    var light := OmniLight3D.new()
    light.position = Vector3(0,1.45,0)
    light.light_color = Color("e3c481")
    light.light_energy = 2.8
    light.omni_range = 7.0
    light.light_size = 0.22
    light.light_volumetric_fog_energy = 0.7
    light.shadow_enabled = true
    body.add_child(light)

    add_child(body)
    return body

func _message(text: String) -> void:
    if is_instance_valid(ui):
        ui.text = text
