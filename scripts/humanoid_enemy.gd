class_name HumanoidEnemy
extends CharacterBody3D

signal killed(position: Vector3)

var target: RiftPlayer
var move_speed := 3.1
var health := 55.0
var attack_damage := 9.0
var attack_cooldown := 0.0
var rng := RandomNumberGenerator.new()
var patrol_origin := Vector3.ZERO
var patrol_target := Vector3.ZERO
var patrol_timer := 0.0
var alerted := false
var visual_root: Node3D
var left_leg: Node3D
var right_leg: Node3D
var left_arm: Node3D
var right_arm: Node3D
var walk_phase := 0.0

func _ready() -> void:
    rng.seed = get_instance_id()
    patrol_origin = global_position
    _build_collision()
    _build_visual()
    _pick_patrol_target()

func _build_collision() -> void:
    var collision := CollisionShape3D.new()
    var capsule := CapsuleShape3D.new()
    capsule.radius = 0.4
    capsule.height = 1.8
    collision.shape = capsule
    collision.position.y = 0.9
    add_child(collision)

func _mat(color: Color, roughness := 0.8) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.roughness = roughness
    return mat

func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color) -> void:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color)
    mi.mesh = mesh
    parent.add_child(mi)

func _build_visual() -> void:
    visual_root = Node3D.new()
    add_child(visual_root)
    var coat := Color("3e332e") if rng.randf() < 0.5 else Color("30373a")
    _box(visual_root, Vector3(0, 1.2, 0), Vector3(0.7, 0.8, 0.38), coat)
    _box(visual_root, Vector3(0, 1.7, 0), Vector3(0.43, 0.36, 0.4), Color("8b7968"))
    _box(visual_root, Vector3(0, 1.92, 0.02), Vector3(0.5, 0.16, 0.46), Color("171b1d"))

    left_leg = Node3D.new(); left_leg.position = Vector3(-0.19, 0.82, 0); visual_root.add_child(left_leg)
    right_leg = Node3D.new(); right_leg.position = Vector3(0.19, 0.82, 0); visual_root.add_child(right_leg)
    _box(left_leg, Vector3(0, -0.38, 0), Vector3(0.24, 0.8, 0.27), Color("25282c"))
    _box(right_leg, Vector3(0, -0.38, 0), Vector3(0.24, 0.8, 0.27), Color("25282c"))

    left_arm = Node3D.new(); left_arm.position = Vector3(-0.47, 1.42, 0); visual_root.add_child(left_arm)
    right_arm = Node3D.new(); right_arm.position = Vector3(0.47, 1.42, 0); visual_root.add_child(right_arm)
    _box(left_arm, Vector3(0, -0.25, 0), Vector3(0.21, 0.66, 0.23), coat)
    _box(right_arm, Vector3(0, -0.25, 0), Vector3(0.21, 0.66, 0.23), coat)

func _physics_process(delta: float) -> void:
    attack_cooldown = maxf(0.0, attack_cooldown - delta)
    if not is_instance_valid(target):
        _patrol(delta)
        return

    var distance := global_position.distance_to(target.global_position)
    var detect_range := 15.0
    if target.flashlight_on:
        detect_range = 38.0
    if distance < 9.0:
        detect_range = maxf(detect_range, 18.0)

    if distance <= detect_range and _has_line_of_sight():
        alerted = true
    elif distance > 52.0:
        alerted = false

    if alerted:
        _chase(delta, distance)
    else:
        _patrol(delta)

    if not is_on_floor():
        velocity.y -= 23.0 * delta
    move_and_slide()
    _animate(delta)

func _has_line_of_sight() -> bool:
    var from := global_position + Vector3(0, 1.45, 0)
    var to := target.global_position + Vector3(0, 1.25, 0)
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [self]
    var hit := get_world_3d().direct_space_state.intersect_ray(query)
    return not hit.is_empty() and hit.collider == target

func _chase(_delta: float, distance: float) -> void:
    var dir := target.global_position - global_position
    dir.y = 0.0
    dir = dir.normalized()
    if distance > 1.55:
        velocity.x = dir.x * move_speed
        velocity.z = dir.z * move_speed
    else:
        velocity.x = move_toward(velocity.x, 0.0, 1.2)
        velocity.z = move_toward(velocity.z, 0.0, 1.2)
        if attack_cooldown <= 0.0:
            target.apply_damage(attack_damage)
            attack_cooldown = 1.15

func _patrol(delta: float) -> void:
    patrol_timer -= delta
    var dir := patrol_target - global_position
    dir.y = 0.0
    if patrol_timer <= 0.0 or dir.length() < 1.2:
        _pick_patrol_target()
        dir = patrol_target - global_position
        dir.y = 0.0
    dir = dir.normalized()
    velocity.x = dir.x * move_speed * 0.48
    velocity.z = dir.z * move_speed * 0.48

func _pick_patrol_target() -> void:
    patrol_timer = rng.randf_range(4.0, 9.0)
    patrol_target = patrol_origin + Vector3(rng.randf_range(-9, 9), 0, rng.randf_range(-9, 9))

func _animate(delta: float) -> void:
    var flat_speed := Vector2(velocity.x, velocity.z).length()
    if flat_speed > 0.1:
        visual_root.rotation.y = lerp_angle(visual_root.rotation.y, atan2(velocity.x, velocity.z), 1.0 - exp(-8.0 * delta))
        walk_phase += delta * (5.0 + flat_speed)
        var swing := sin(walk_phase) * 0.48
        left_leg.rotation.x = swing
        right_leg.rotation.x = -swing
        left_arm.rotation.x = -swing * 0.6
        right_arm.rotation.x = swing * 0.6

func take_damage(amount: float) -> void:
    health -= amount
    alerted = true
    if health <= 0.0:
        killed.emit(global_position)
        queue_free()
