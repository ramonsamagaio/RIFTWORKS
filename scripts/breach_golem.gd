class_name BreachGolem
extends CharacterBody3D

var target: RiftPlayer
var health := 140.0
var move_speed := 2.1
var attack_cooldown := 0.0
var visual_root: Node3D
var left_arm: Node3D
var right_arm: Node3D
var left_leg: Node3D
var right_leg: Node3D
var core_light: OmniLight3D
var phase := 0.0

func _ready() -> void:
    _build_collision()
    _build_visual()

func _physics_process(delta: float) -> void:
    attack_cooldown = maxf(0.0, attack_cooldown - delta)
    if not is_instance_valid(target):
        return
    var to_target := target.global_position - global_position
    to_target.y = 0.0
    var distance := to_target.length()
    if distance < 24.0:
        var dir := to_target.normalized()
        if distance > 2.3:
            velocity.x = dir.x * move_speed
            velocity.z = dir.z * move_speed
        else:
            velocity.x = move_toward(velocity.x, 0.0, 0.7)
            velocity.z = move_toward(velocity.z, 0.0, 0.7)
            if attack_cooldown <= 0.0:
                target.apply_damage(18.0)
                attack_cooldown = 1.8
        visual_root.rotation.y = lerp_angle(visual_root.rotation.y, atan2(dir.x, dir.z), 1.0 - exp(-5.0 * delta))
    else:
        velocity.x = move_toward(velocity.x, 0.0, 0.3)
        velocity.z = move_toward(velocity.z, 0.0, 0.3)
    if not is_on_floor():
        velocity.y -= 24.0 * delta
    move_and_slide()
    _animate(delta)

func _mat(color: Color, emission := Color.BLACK, emission_energy := 0.0) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.roughness = 0.78
    mat.metallic = 0.18
    if emission_energy > 0.0:
        mat.emission_enabled = true
        mat.emission = emission
        mat.emission_energy_multiplier = emission_energy
    return mat

func _block(parent: Node3D, pos: Vector3, size: Vector3, color: Color) -> MeshInstance3D:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var mesh := BoxMesh.new()
    mesh.size = size
    mesh.material = _mat(color)
    mi.mesh = mesh
    parent.add_child(mi)
    return mi

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var capsule := CapsuleShape3D.new()
    capsule.radius = 0.82
    capsule.height = 2.9
    cs.shape = capsule
    cs.position.y = 1.45
    add_child(cs)

func _build_visual() -> void:
    visual_root = Node3D.new()
    add_child(visual_root)
    _block(visual_root, Vector3(0,1.65,0), Vector3(1.45,1.35,0.95), Color("34313c"))
    _block(visual_root, Vector3(0,2.65,0), Vector3(0.88,0.68,0.72), Color("292732"))

    left_arm = Node3D.new(); left_arm.position = Vector3(-0.98,2.0,0); visual_root.add_child(left_arm)
    right_arm = Node3D.new(); right_arm.position = Vector3(0.98,2.0,0); visual_root.add_child(right_arm)
    _block(left_arm, Vector3(0,-0.42,0), Vector3(0.52,1.55,0.58), Color("3b3745"))
    _block(right_arm, Vector3(0,-0.42,0), Vector3(0.52,1.55,0.58), Color("3b3745"))

    left_leg = Node3D.new(); left_leg.position = Vector3(-0.42,1.05,0); visual_root.add_child(left_leg)
    right_leg = Node3D.new(); right_leg.position = Vector3(0.42,1.05,0); visual_root.add_child(right_leg)
    _block(left_leg, Vector3(0,-0.52,0), Vector3(0.55,1.15,0.62), Color("2f2d36"))
    _block(right_leg, Vector3(0,-0.52,0), Vector3(0.55,1.15,0.62), Color("2f2d36"))

    var core := MeshInstance3D.new()
    core.position = Vector3(0,1.72,-0.52)
    var core_mesh := CylinderMesh.new()
    core_mesh.top_radius = 0.18
    core_mesh.bottom_radius = 0.28
    core_mesh.height = 0.35
    core_mesh.radial_segments = 6
    core_mesh.material = _mat(Color("5d4773"), Color("ae79dc"), 3.5)
    core.mesh = core_mesh
    core.rotation_degrees.x = 90.0
    visual_root.add_child(core)

    core_light = OmniLight3D.new()
    core_light.position = Vector3(0,1.72,-0.7)
    core_light.light_color = Color("a96fd4")
    core_light.light_energy = 2.8
    core_light.omni_range = 6.0
    core_light.light_volumetric_fog_energy = 0.9
    visual_root.add_child(core_light)

func _animate(delta: float) -> void:
    var speed := Vector2(velocity.x, velocity.z).length()
    phase += delta * (2.6 + speed * 1.3)
    var swing := sin(phase) * 0.34 if speed > 0.1 else sin(phase * 0.35) * 0.04
    left_leg.rotation.x = swing
    right_leg.rotation.x = -swing
    left_arm.rotation.x = -swing * 0.65
    right_arm.rotation.x = swing * 0.65
    if is_instance_valid(core_light):
        core_light.light_energy = 2.6 + sin(phase * 1.7) * 0.45

func take_damage(amount: float) -> void:
    health -= amount
    if health <= 0.0:
        var core_drop := SalvageProp.new()
        core_drop.configure("breach_core", "Golem Breach Core", 1, 1, Color("ad78d8"))
        core_drop.global_position = global_position + Vector3.UP * 1.1
        get_parent().add_child(core_drop)
        queue_free()
