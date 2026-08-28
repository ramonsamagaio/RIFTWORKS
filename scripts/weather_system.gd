class_name WeatherSystem
extends Node3D

var player: RiftPlayer
var rain: GPUParticles3D
var lightning: OmniLight3D
var lightning_timer := 8.0
var rng := RandomNumberGenerator.new()

func _ready() -> void:
    rng.seed = 774921
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    _build_rain()
    _build_lightning()
    lightning_timer = rng.randf_range(8.0,18.0)

func _process(delta: float) -> void:
    if is_instance_valid(player) and is_instance_valid(rain):
        rain.global_position = player.global_position + Vector3(0,18,0)
    lightning_timer -= delta
    if lightning_timer <= 0.0:
        lightning_timer = rng.randf_range(13.0,31.0)
        _flash_lightning()

func _build_rain() -> void:
    rain = GPUParticles3D.new()
    rain.name = "LocalRainField"
    rain.amount = 1500
    rain.lifetime = 1.45
    rain.preprocess = 1.4
    rain.randomness = 0.32
    rain.visibility_aabb = AABB(Vector3(-34,-30,-34), Vector3(68,42,68))

    var process := ParticleProcessMaterial.new()
    process.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
    process.emission_box_extents = Vector3(31,1.2,31)
    process.direction = Vector3(0,-1,0)
    process.spread = 4.0
    process.initial_velocity_min = 24.0
    process.initial_velocity_max = 34.0
    process.gravity = Vector3(1.2,-12.0,0.6)
    process.scale_min = 0.7
    process.scale_max = 1.25
    rain.process_material = process

    var drop := BoxMesh.new()
    drop.size = Vector3(0.018,0.42,0.018)
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color(0.52,0.62,0.72,0.55)
    mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    mat.vertex_color_use_as_albedo = true
    drop.material = mat
    rain.draw_pass_1 = drop
    add_child(rain)

func _build_lightning() -> void:
    lightning = OmniLight3D.new()
    lightning.name = "StormFlash"
    lightning.light_color = Color("b9c8e8")
    lightning.light_energy = 0.0
    lightning.omni_range = 95.0
    lightning.light_size = 18.0
    lightning.light_volumetric_fog_energy = 1.6
    lightning.shadow_enabled = false
    add_child(lightning)

func _flash_lightning() -> void:
    if not is_instance_valid(player) or not is_instance_valid(lightning):
        return
    lightning.global_position = player.global_position + Vector3(rng.randf_range(-25,25),28,rng.randf_range(-25,25))
    lightning.light_energy = rng.randf_range(5.5,8.5)
    var tween := create_tween()
    tween.tween_property(lightning,"light_energy",0.3,0.055)
    tween.tween_interval(rng.randf_range(0.04,0.12))
    tween.tween_property(lightning,"light_energy",rng.randf_range(2.5,4.5),0.025)
    tween.tween_property(lightning,"light_energy",0.0,0.16)
