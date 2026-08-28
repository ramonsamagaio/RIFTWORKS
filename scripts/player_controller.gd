class_name RiftPlayer
extends CharacterBody3D

signal build_requested(kind: String, position: Vector3)
signal died

const WALK_SPEED := 5.4
const SPRINT_SPEED := 8.2
const ACCELERATION := 18.0
const GRAVITY := 24.0
const INTERACT_DISTANCE := 5.5
const EYE_HEIGHT := 1.66
const CAMERA_FOV := 78.0
const WALK_BOB_SPEED := 9.2
const WALK_BOB_AMOUNT := 0.028
const SPRINT_BOB_SPEED := 12.8
const SPRINT_BOB_AMOUNT := 0.043

var yaw := 0.0
var pitch := -0.08
var health := 100.0
var flashlight_battery := 100.0
var flashlight_on := true
var scrap := 14
var components: Dictionary = {
    "battery_cell": 2,
    "cable": 2,
    "electronics": 2,
    "motor": 1,
    "fuel": 1
}

var camera: Camera3D
var flashlight: SpotLight3D
var flashlight_spill: SpotLight3D
var visual_root: Node3D
var left_leg: Node3D
var right_leg: Node3D
var left_arm: Node3D
var right_arm: Node3D
var walk_phase := 0.0
var view_bob_phase := 0.0
var interaction_prompt := ""

func _ready() -> void:
    add_to_group("player")
    _force_fullscreen()
    _build_collision()
    _build_shadow_body()
    _build_camera()
    _build_flashlight()
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _force_fullscreen() -> void:
    if DisplayServer.get_name() != "headless":
        DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)

func _build_collision() -> void:
    var collision := CollisionShape3D.new()
    var capsule := CapsuleShape3D.new()
    capsule.radius = 0.42
    capsule.height = 1.82
    collision.shape = capsule
    collision.position.y = 0.91
    add_child(collision)

func _mat(color: Color, metallic := 0.0, roughness := 0.72) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _shadow_box(parent: Node3D, pos: Vector3, size: Vector3, color: Color) -> MeshInstance3D:
    var mi := MeshInstance3D.new()
    mi.position = pos
    var box := BoxMesh.new()
    box.size = size
    box.material = _mat(color)
    mi.mesh = box
    # The first-person body exists only to cast a convincing player shadow.
    # It is deliberately invisible to the player camera to eliminate self-clipping.
    mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_SHADOWS_ONLY
    parent.add_child(mi)
    return mi

func _build_shadow_body() -> void:
    visual_root = Node3D.new()
    visual_root.name = "FirstPersonShadowBody"
    add_child(visual_root)
    _shadow_box(visual_root, Vector3(0, 1.22, 0), Vector3(0.72, 0.82, 0.38), Color("49555d"))
    _shadow_box(visual_root, Vector3(0, 1.72, 0), Vector3(0.46, 0.36, 0.42), Color("b19c86"))
    _shadow_box(visual_root, Vector3(0, 1.91, 0.03), Vector3(0.56, 0.14, 0.5), Color("242a30"))

    left_leg = Node3D.new()
    left_leg.position = Vector3(-0.2, 0.82, 0)
    visual_root.add_child(left_leg)
    right_leg = Node3D.new()
    right_leg.position = Vector3(0.2, 0.82, 0)
    visual_root.add_child(right_leg)
    _shadow_box(left_leg, Vector3(0, -0.38, 0), Vector3(0.25, 0.82, 0.28), Color("293039"))
    _shadow_box(right_leg, Vector3(0, -0.38, 0), Vector3(0.25, 0.82, 0.28), Color("293039"))

    left_arm = Node3D.new()
    left_arm.position = Vector3(-0.48, 1.43, 0)
    visual_root.add_child(left_arm)
    right_arm = Node3D.new()
    right_arm.position = Vector3(0.48, 1.43, 0)
    visual_root.add_child(right_arm)
    _shadow_box(left_arm, Vector3(0, -0.26, 0), Vector3(0.22, 0.68, 0.24), Color("465159"))
    _shadow_box(right_arm, Vector3(0, -0.26, 0), Vector3(0.22, 0.68, 0.24), Color("465159"))

func _build_camera() -> void:
    camera = Camera3D.new()
    camera.name = "FirstPersonCamera"
    camera.current = true
    camera.fov = CAMERA_FOV
    camera.near = 0.045
    camera.far = 900.0
    add_child(camera)
    _update_camera(0.0)

func _build_flashlight() -> void:
    flashlight = SpotLight3D.new()
    flashlight.name = "FlashlightMain"
    flashlight.light_color = Color(1.0, 0.91, 0.78)
    flashlight.light_energy = 92.0
    flashlight.light_specular = 1.0
    flashlight.light_size = 0.12
    flashlight.light_volumetric_fog_energy = 1.7
    flashlight.spot_range = 54.0
    flashlight.spot_angle = 21.5
    flashlight.spot_attenuation = 2.0
    flashlight.spot_angle_attenuation = 4.2
    flashlight.shadow_enabled = true
    flashlight.shadow_bias = 0.02
    flashlight.shadow_normal_bias = 0.38
    flashlight.shadow_blur = 1.35
    var cookie: Resource = load("res://assets/textures/flashlight_cookie.svg")
    if cookie is Texture2D:
        flashlight.light_projector = cookie
    add_child(flashlight)

    flashlight_spill = SpotLight3D.new()
    flashlight_spill.name = "FlashlightSpill"
    flashlight_spill.light_color = Color(1.0, 0.88, 0.73)
    flashlight_spill.light_energy = 5.5
    flashlight_spill.light_specular = 0.55
    flashlight_spill.light_volumetric_fog_energy = 0.18
    flashlight_spill.spot_range = 18.0
    flashlight_spill.spot_angle = 43.0
    flashlight_spill.spot_attenuation = 1.55
    flashlight_spill.spot_angle_attenuation = 2.4
    flashlight_spill.shadow_enabled = false
    add_child(flashlight_spill)

func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        yaw -= event.relative.x * 0.00225
        pitch = clampf(pitch - event.relative.y * 0.0021, -1.46, 1.46)

    if event is InputEventKey and event.pressed and not event.echo:
        match event.keycode:
            KEY_ESCAPE:
                Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
            KEY_F:
                set_flashlight(not flashlight_on)
            KEY_E:
                _try_interact()
            KEY_R:
                _use_battery_cell()
            KEY_B:
                _request_build("floodlight")
            KEY_G:
                _request_build("generator")
            KEY_T:
                _request_build("battery")

func _physics_process(delta: float) -> void:
    var input := Vector2.ZERO
    if Input.is_key_pressed(KEY_A):
        input.x -= 1.0
    if Input.is_key_pressed(KEY_D):
        input.x += 1.0
    if Input.is_key_pressed(KEY_W):
        input.y -= 1.0
    if Input.is_key_pressed(KEY_S):
        input.y += 1.0
    input = input.normalized()

    var yaw_basis := Basis(Vector3.UP, yaw)
    var desired := yaw_basis * Vector3(input.x, 0, input.y)
    var speed := SPRINT_SPEED if Input.is_key_pressed(KEY_SHIFT) else WALK_SPEED
    var target_velocity := desired * speed
    velocity.x = move_toward(velocity.x, target_velocity.x, ACCELERATION * delta)
    velocity.z = move_toward(velocity.z, target_velocity.z, ACCELERATION * delta)

    if not is_on_floor():
        velocity.y -= GRAVITY * delta
    elif Input.is_key_pressed(KEY_SPACE):
        velocity.y = 7.2

    move_and_slide()
    _update_shadow_body(delta)
    _update_camera(delta)
    _update_flashlight(delta)
    _update_interaction_prompt()

func _process(delta: float) -> void:
    if flashlight_on:
        flashlight_battery = maxf(0.0, flashlight_battery - delta * 0.24)
        if flashlight_battery <= 0.0:
            set_flashlight(false)

func _update_shadow_body(delta: float) -> void:
    if not is_instance_valid(visual_root):
        return
    visual_root.rotation.y = yaw
    var flat_speed := Vector2(velocity.x, velocity.z).length()
    if flat_speed > 0.15 and is_on_floor():
        walk_phase += delta * (7.0 + flat_speed * 0.8)
        var swing := sin(walk_phase) * 0.52
        left_leg.rotation.x = swing
        right_leg.rotation.x = -swing
        left_arm.rotation.x = -swing * 0.65
        right_arm.rotation.x = swing * 0.65
    else:
        left_leg.rotation.x = lerpf(left_leg.rotation.x, 0.0, 1.0 - exp(-8.0 * delta))
        right_leg.rotation.x = lerpf(right_leg.rotation.x, 0.0, 1.0 - exp(-8.0 * delta))
        left_arm.rotation.x = lerpf(left_arm.rotation.x, 0.0, 1.0 - exp(-8.0 * delta))
        right_arm.rotation.x = lerpf(right_arm.rotation.x, 0.0, 1.0 - exp(-8.0 * delta))

func _camera_bob(delta: float) -> Vector3:
    var flat_speed := Vector2(velocity.x, velocity.z).length()
    if flat_speed < 0.2 or not is_on_floor():
        view_bob_phase = lerpf(view_bob_phase, 0.0, minf(1.0, delta * 5.0))
        return Vector3.ZERO

    var sprinting := Input.is_key_pressed(KEY_SHIFT) and flat_speed > WALK_SPEED + 0.5
    var bob_speed := SPRINT_BOB_SPEED if sprinting else WALK_BOB_SPEED
    var amount := SPRINT_BOB_AMOUNT if sprinting else WALK_BOB_AMOUNT
    view_bob_phase += delta * bob_speed
    return Vector3(
        cos(view_bob_phase * 0.5) * amount * 0.48,
        abs(sin(view_bob_phase)) * amount,
        0.0
    )

func _update_camera(delta: float) -> void:
    if not is_instance_valid(camera):
        return
    var bob := _camera_bob(delta)
    var view_basis := Basis(Vector3.UP, yaw) * Basis(Vector3.RIGHT, pitch)
    var local_bob := Basis(Vector3.UP, yaw) * bob
    var eye_position := global_position + Vector3(0, EYE_HEIGHT, 0) + local_bob
    camera.global_transform = Transform3D(view_basis, eye_position)

func _update_flashlight(_delta: float) -> void:
    if not is_instance_valid(camera):
        return
    # Slightly off-axis mount keeps the beam feeling like a held flashlight instead
    # of a perfectly centered game-camera spotlight.
    var mount_offset := camera.global_basis * Vector3(0.17, -0.13, -0.09)
    var mount_position := camera.global_position + mount_offset
    flashlight.global_position = mount_position
    flashlight_spill.global_position = mount_position

    var ray_from := camera.global_position
    var ray_to := ray_from + (-camera.global_basis.z) * 80.0
    var query := PhysicsRayQueryParameters3D.create(ray_from, ray_to)
    query.exclude = [self]
    var hit := get_world_3d().direct_space_state.intersect_ray(query)
    var target: Vector3 = hit.position if not hit.is_empty() else ray_to
    flashlight.look_at(target, Vector3.UP)
    flashlight_spill.look_at(target, Vector3.UP)

    var low_factor := clampf(flashlight_battery / 10.0, 0.0, 1.0)
    var instability := 1.0
    if flashlight_battery < 10.0:
        instability = lerpf(0.58 + sin(Time.get_ticks_msec() * 0.021) * 0.22, 1.0, low_factor)
    flashlight.light_energy = 92.0 * instability
    flashlight_spill.light_energy = 5.5 * instability

func set_flashlight(value: bool) -> void:
    flashlight_on = value and flashlight_battery > 0.0
    if is_instance_valid(flashlight):
        flashlight.visible = flashlight_on
    if is_instance_valid(flashlight_spill):
        flashlight_spill.visible = flashlight_on

func _interaction_hit() -> Dictionary:
    if not is_instance_valid(camera):
        return {}
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * INTERACT_DISTANCE
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [self]
    return get_world_3d().direct_space_state.intersect_ray(query)

func _update_interaction_prompt() -> void:
    interaction_prompt = ""
    var hit := _interaction_hit()
    if hit.is_empty():
        return
    var collider = hit.collider
    if collider and collider.has_method("get_prompt_text"):
        interaction_prompt = collider.get_prompt_text()

func _try_interact() -> void:
    var hit := _interaction_hit()
    if hit.is_empty():
        return
    var collider = hit.collider
    if collider and collider.has_method("interact"):
        collider.interact(self)

func _get_build_position() -> Vector3:
    var from := camera.global_position
    var to := from + (-camera.global_basis.z) * 9.0
    var query := PhysicsRayQueryParameters3D.create(from, to)
    query.exclude = [self]
    var hit := get_world_3d().direct_space_state.intersect_ray(query)
    if not hit.is_empty():
        return hit.position + Vector3.UP * 0.05
    return global_position + Basis(Vector3.UP, yaw) * Vector3(0, 0, -3.5)

func _request_build(kind: String) -> void:
    var cost: Dictionary = {}
    match kind:
        "floodlight":
            cost = {"scrap": 3, "cable": 1, "electronics": 1}
        "generator":
            cost = {"scrap": 5, "motor": 1, "fuel": 1}
        "battery":
            cost = {"scrap": 4, "battery_cell": 2}
        _:
            return
    if not _can_afford(cost):
        return
    _pay_cost(cost)
    build_requested.emit(kind, _get_build_position())

func _can_afford(cost: Dictionary) -> bool:
    for key in cost:
        var required: int = int(cost[key])
        if key == "scrap":
            if scrap < required:
                return false
        elif int(components.get(key, 0)) < required:
            return false
    return true

func _pay_cost(cost: Dictionary) -> void:
    for key in cost:
        var amount: int = int(cost[key])
        if key == "scrap":
            scrap -= amount
        else:
            components[key] = int(components.get(key, 0)) - amount

func add_component(item_id: String, amount: int) -> void:
    if item_id == "scrap":
        scrap += amount
    else:
        components[item_id] = int(components.get(item_id, 0)) + amount

func _use_battery_cell() -> void:
    if flashlight_battery >= 99.0 or int(components.get("battery_cell", 0)) <= 0:
        return
    components["battery_cell"] = int(components.get("battery_cell", 0)) - 1
    flashlight_battery = minf(100.0, flashlight_battery + 42.0)
    if flashlight_battery > 0.0 and not flashlight_on:
        set_flashlight(true)

func apply_damage(amount: float) -> void:
    health = maxf(0.0, health - amount)
    if health <= 0.0:
        died.emit()

func get_hud_status() -> String:
    return "HP %03d  |  LIGHT %03d%%  |  SCRAP %02d  |  MOTOR %d  CABLE %d  ELEC %d  CELLS %d" % [
        int(health), int(flashlight_battery), scrap,
        int(components.get("motor", 0)), int(components.get("cable", 0)),
        int(components.get("electronics", 0)), int(components.get("battery_cell", 0))
    ]
