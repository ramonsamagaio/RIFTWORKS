class_name CombatSystem
extends Node

const RIFLE_RANGE := 95.0
const RIFLE_DAMAGE := 22.0

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
        _fire_rifle()

func _fire_rifle() -> void:
    cooldown = 0.20
    var camera := player.camera
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * RIFLE_RANGE
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [player]
    query.collide_with_areas = true
    var hit := player.get_world_3d().direct_space_state.intersect_ray(query)
    var end := to
    if not hit.is_empty():
        end = hit.position
    _muzzle_flash(from + (-camera.global_basis.z) * 0.7)
    _tracer(from + (-camera.global_basis.z) * 0.65, end)

    if hit.is_empty():
        _flash_feedback("MISS", Color("87949c"))
        return

    var collider = hit.collider
    if collider and collider.has_method("take_hit"):
        var result = collider.call("take_hit", RIFLE_DAMAGE, hit.position)
        _flash_feedback(str(result), Color("d7b7f2") if str(result).contains("DESTROYED") else Color("e9d7b8"))
    elif collider and collider.has_method("take_damage"):
        collider.call("take_damage", RIFLE_DAMAGE)
        _flash_feedback("HIT", Color("e6c7a0"))
    else:
        _flash_feedback("IMPACT", Color("9aa8b0"))

func _tracer(from: Vector3, to: Vector3) -> void:
    var delta := to - from
    if delta.length() <= 0.02:
        return
    var tracer := MeshInstance3D.new()
    tracer.global_position = (from + to) * 0.5
    var mesh := CylinderMesh.new()
    mesh.top_radius = 0.006
    mesh.bottom_radius = 0.006
    mesh.height = delta.length()
    mesh.radial_segments = 5
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("e5c896")
    mat.emission_enabled = true
    mat.emission = Color("e5c896")
    mat.emission_energy_multiplier = 2.8
    mesh.material = mat
    tracer.mesh = mesh
    tracer.quaternion = Quaternion(Vector3.UP, delta.normalized())
    get_parent().add_child(tracer)
    var tween := create_tween()
    tween.tween_interval(0.035)
    tween.tween_callback(tracer.queue_free)

func _muzzle_flash(position: Vector3) -> void:
    var flash := OmniLight3D.new()
    flash.global_position = position
    flash.light_color = Color(1.0,0.73,0.42)
    flash.light_energy = 5.5
    flash.omni_range = 4.2
    flash.light_volumetric_fog_energy = 1.1
    get_parent().add_child(flash)
    var tween := create_tween()
    tween.tween_property(flash, "light_energy", 0.0, 0.055)
    tween.tween_callback(flash.queue_free)

func _make_feedback() -> void:
    var layer := CanvasLayer.new()
    layer.layer = 4
    add_child(layer)
    feedback = Label.new()
    feedback.position = Vector2(550, 388)
    feedback.size = Vector2(190, 30)
    feedback.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    feedback.add_theme_font_size_override("font_size", 13)
    feedback.modulate.a = 0.0
    layer.add_child(feedback)

func _flash_feedback(text: String, color: Color) -> void:
    feedback.text = text
    feedback.add_theme_color_override("font_color", color)
    feedback.modulate.a = 1.0
    var tween := create_tween()
    tween.tween_property(feedback, "modulate:a", 0.0, 0.34)
