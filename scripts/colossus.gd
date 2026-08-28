class_name ColossusWalker
extends CharacterBody3D

var move_speed := 1.05
var route_radius := 46.0
var route_center := Vector3.ZERO
var route_angle := 0.0
var walk_phase := 0.0
var left_leg: Node3D
var right_leg: Node3D
var left_arm: Node3D
var right_arm: Node3D
var head: Node3D
var pulse_light: OmniLight3D

func _ready() -> void:
    route_center = global_position
    _build_collision()
    _build_body()

func _mat(color: Color, metallic := 0.0, roughness := 0.75) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _box(parent: Node3D, pos: Vector3, size: Vector3, color: Color, metallic := 0.0) -> MeshInstance3D:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var box := BoxMesh.new()
    box.size = size
    box.material = _mat(color, metallic)
    mi.mesh = box
    mi.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    parent.add_child(mi)
    return mi

func _build_collision() -> void:
    var collision := CollisionShape3D.new()
    var capsule := CapsuleShape3D.new()
    capsule.radius = 4.2
    capsule.height = 18.0
    collision.shape = capsule
    collision.position.y = 10.0
    add_child(collision)

func _build_body() -> void:
    var root := Node3D.new()
    root.name = "ColossusRig"
    add_child(root)

    left_leg = Node3D.new(); left_leg.position = Vector3(-3.4, 13.0, 0); root.add_child(left_leg)
    right_leg = Node3D.new(); right_leg.position = Vector3(3.4, 13.0, 0); root.add_child(right_leg)
    _box(left_leg, Vector3(0, -6.5, 0), Vector3(3.1, 13.0, 3.1), Color("181a20"), 0.28)
    _box(right_leg, Vector3(0, -6.5, 0), Vector3(3.1, 13.0, 3.1), Color("181a20"), 0.28)
    _box(left_leg, Vector3(0, -13.2, -0.8), Vector3(4.8, 1.2, 6.2), Color("111319"), 0.42)
    _box(right_leg, Vector3(0, -13.2, -0.8), Vector3(4.8, 1.2, 6.2), Color("111319"), 0.42)

    var torso := Node3D.new(); torso.position = Vector3(0, 22.5, 0); root.add_child(torso)
    _box(torso, Vector3.ZERO, Vector3(12.5, 9.0, 7.2), Color("20222a"), 0.35)
    _box(torso, Vector3(0, 1.4, -3.9), Vector3(8.0, 3.4, 0.9), Color("2c2a36"), 0.48)

    left_arm = Node3D.new(); left_arm.position = Vector3(-7.1, 24.0, 0); root.add_child(left_arm)
    right_arm = Node3D.new(); right_arm.position = Vector3(7.1, 24.0, 0); root.add_child(right_arm)
    _box(left_arm, Vector3(0, -5.1, 0), Vector3(2.4, 11.0, 2.6), Color("171920"), 0.25)
    _box(right_arm, Vector3(0, -5.1, 0), Vector3(2.4, 11.0, 2.6), Color("171920"), 0.25)

    head = Node3D.new(); head.position = Vector3(0, 30.2, -0.3); root.add_child(head)
    _box(head, Vector3.ZERO, Vector3(6.6, 5.0, 5.4), Color("242630"), 0.34)
    _box(head, Vector3(0, 0.4, -2.9), Vector3(3.4, 1.0, 0.45), Color("b59ee9"), 0.12)

    pulse_light = OmniLight3D.new()
    pulse_light.position = Vector3(0, 0.4, -3.3)
    pulse_light.light_color = Color("c6afff")
    pulse_light.light_energy = 9.0
    pulse_light.omni_range = 34.0
    pulse_light.light_volumetric_fog_energy = 2.2
    head.add_child(pulse_light)

func _physics_process(delta: float) -> void:
    route_angle += delta * 0.018
    var desired_point := route_center + Vector3(cos(route_angle), 0, sin(route_angle)) * route_radius
    var dir := desired_point - global_position
    dir.y = 0.0
    if dir.length() > 0.5:
        dir = dir.normalized()
        velocity.x = dir.x * move_speed
        velocity.z = dir.z * move_speed
        rotation.y = lerp_angle(rotation.y, atan2(dir.x, dir.z), 1.0 - exp(-0.9 * delta))
    if not is_on_floor():
        velocity.y -= 22.0 * delta
    move_and_slide()

    walk_phase += delta * 1.32
    var leg_swing := sin(walk_phase) * 0.18
    left_leg.rotation.x = leg_swing
    right_leg.rotation.x = -leg_swing
    left_arm.rotation.x = -leg_swing * 0.7
    right_arm.rotation.x = leg_swing * 0.7
    head.rotation.y = sin(walk_phase * 0.23) * 0.13

    var footbeat := pow(maxf(0.0, sin(walk_phase * 2.0)), 14.0)
    pulse_light.light_energy = 8.0 + footbeat * 6.0
