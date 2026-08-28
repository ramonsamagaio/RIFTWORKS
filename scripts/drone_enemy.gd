class_name DroneEnemy
extends CharacterBody3D

var target: RiftPlayer
var hover_origin := Vector3.ZERO
var phase := 0.0
var health := 36.0
var attack_cooldown := 0.0
var rotor: Node3D
var eye: OmniLight3D

func _ready() -> void:
    hover_origin = global_position
    _build_collision()
    _build_visual()

func _mat(color: Color, metallic := 0.7, roughness := 0.42) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
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

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := SphereShape3D.new()
    shape.radius = 0.62
    cs.shape = shape
    add_child(cs)

func _build_visual() -> void:
    rotor = Node3D.new()
    add_child(rotor)
    _box(rotor, Vector3.ZERO, Vector3(1.15,0.42,0.88), Color("343a42"))
    _box(rotor, Vector3(0.92,0,0), Vector3(0.75,0.1,0.13), Color("262c31"))
    _box(rotor, Vector3(-0.92,0,0), Vector3(0.75,0.1,0.13), Color("262c31"))
    _box(rotor, Vector3(0,0,0.78), Vector3(0.13,0.1,0.72), Color("262c31"))
    _box(rotor, Vector3(0,0,-0.78), Vector3(0.13,0.1,0.72), Color("262c31"))

    eye = OmniLight3D.new()
    eye.position = Vector3(0,-0.08,-0.5)
    eye.light_color = Color("d6554b")
    eye.light_energy = 2.3
    eye.omni_range = 5.5
    eye.light_volumetric_fog_energy = 0.6
    add_child(eye)

func _physics_process(delta: float) -> void:
    phase += delta
    attack_cooldown = maxf(0.0, attack_cooldown - delta)
    rotor.rotation.y += delta * 1.8

    if not is_instance_valid(target):
        global_position.y = hover_origin.y + sin(phase*1.4)*0.18
        return

    var to_target := target.global_position + Vector3(0,1.2,0) - global_position
    var distance := to_target.length()
    var detection := 12.0 if not target.flashlight_on else 25.0
    if distance < detection:
        var desired := to_target.normalized() * 4.1
        velocity = velocity.lerp(desired, 1.0 - exp(-3.6 * delta))
        if distance < 1.7 and attack_cooldown <= 0.0:
            target.apply_damage(6.0)
            attack_cooldown = 0.85
    else:
        var home := hover_origin + Vector3(sin(phase*0.4)*3.0, sin(phase*1.3)*0.25, cos(phase*0.4)*3.0)
        velocity = velocity.lerp((home-global_position).normalized()*1.6, 1.0-exp(-2.0*delta))

    move_and_slide()
    eye.light_energy = 2.1 + (sin(phase*5.0)+1.0)*0.35

func take_damage(amount: float) -> void:
    health -= amount
    if health <= 0.0:
        queue_free()
