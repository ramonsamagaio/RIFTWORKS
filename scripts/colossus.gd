class_name ColossusWalker
extends CharacterBody3D

signal weakpoint_destroyed(kind: String)
signal killed(position: Vector3)

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
var weakpoint_hp := {"head":45.0,"torso":60.0,"legs":70.0}
var destroyed := {"head":false,"torso":false,"legs":false}
var dead := false

func _ready() -> void:
    add_to_group("colossi")
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
    capsule.radius = 5.1
    capsule.height = 34.0
    collision.shape = capsule
    collision.position.y = 16.7
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
    _box(torso, Vector3(0, 1.4, -3.9), Vector3(8.0, 3.4, 0.9), Color("5f4e73"), 0.42)

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
    if dead:
        return
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
    pulse_light.light_energy = (4.0 if destroyed["head"] else 8.0) + footbeat * (2.0 if destroyed["head"] else 6.0)

func take_hit(amount: float, world_hit_position: Vector3) -> String:
    if dead:
        return "DEAD"
    var local_hit := to_local(world_hit_position)
    var weakpoint := "armor"
    if local_hit.y >= 26.0:
        weakpoint = "head"
    elif local_hit.y >= 18.0 and local_hit.y <= 26.0:
        weakpoint = "torso"
    elif local_hit.y <= 9.5:
        weakpoint = "legs"

    if weakpoint == "armor":
        return "ARMORED"
    if bool(destroyed[weakpoint]):
        return "DESTROYED"

    weakpoint_hp[weakpoint] = float(weakpoint_hp[weakpoint]) - amount
    if float(weakpoint_hp[weakpoint]) <= 0.0:
        destroyed[weakpoint] = true
        weakpoint_destroyed.emit(weakpoint)
        _on_weakpoint_destroyed(weakpoint)
        if bool(destroyed["head"]) and bool(destroyed["torso"]) and bool(destroyed["legs"]):
            _die()
        return "%s DESTROYED" % weakpoint.to_upper()
    return "%s %.0f" % [weakpoint.to_upper(), maxf(0.0,float(weakpoint_hp[weakpoint]))]

func take_damage(_amount: float) -> void:
    # Colossi deliberately ignore generic damage. Their vulnerable zones must be targeted.
    pass

func _on_weakpoint_destroyed(kind: String) -> void:
    match kind:
        "legs":
            move_speed *= 0.34
        "torso":
            left_arm.rotation.z = 0.42
            right_arm.rotation.z = -0.42
        "head":
            pulse_light.light_color = Color("6c4b72")
            pulse_light.light_volumetric_fog_energy = 0.45

func _die() -> void:
    if dead:
        return
    dead = true
    velocity = Vector3.ZERO
    killed.emit(global_position)
    pulse_light.light_color = Color("5d406c")
    pulse_light.light_energy = 2.0
    _drop_harvest()
    var tween := create_tween()
    tween.tween_property(self, "rotation:z", deg_to_rad(82.0), 2.8).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
    tween.tween_interval(1.5)
    tween.tween_callback(queue_free)

func _drop_harvest() -> void:
    var parent := get_parent()
    if parent == null:
        return
    var core := SalvageProp.new()
    core.configure("breach_core", "Walker Bioelectric Core", 2, 1, Color("b180e4"))
    parent.add_child(core)
    core.global_position = global_position + Vector3(0,1.0,0)
    var plate := SalvageProp.new()
    plate.configure("scrap", "Colossus Carapace", 14, 1, Color("6c6574"))
    parent.add_child(plate)
    plate.global_position = global_position + Vector3(2.0,0.8,1.5)
