class_name HarpoonDevice
extends StaticBody3D

var enabled := false
var target: ColossusWalker
var target_local_point := Vector3.ZERO
var cable: MeshInstance3D
var cable_mesh: CylinderMesh
var status_light: OmniLight3D
var stress_timer := 0.0

func _ready() -> void:
    add_to_group("logic_receivers")
    _build_collision()
    _build_visual()

func _physics_process(delta: float) -> void:
    if not is_instance_valid(target):
        if enabled:
            _detach()
        return
    _update_cable()
    var anchor := global_position + global_basis * Vector3(0,0.9,-0.9)
    var target_point := target.to_global(target_local_point)
    var distance := anchor.distance_to(target_point)
    if distance > 88.0:
        _detach()
        return
    stress_timer -= delta
    if stress_timer <= 0.0:
        stress_timer = 0.65
        var count := int(target.get_meta("harpoon_count",1))
        _recompute_target_speed(target,count)
        if count >= 3 and not bool(target.destroyed.get("legs",false)):
            var stress_hit := target.global_position + Vector3(0,4.0,0)
            target.take_hit(1.1 * float(count),stress_hit)

func _build_collision() -> void:
    var cs := CollisionShape3D.new()
    var shape := BoxShape3D.new()
    shape.size = Vector3(1.15,1.25,1.8)
    cs.shape = shape
    cs.position = Vector3(0,0.62,0)
    add_child(cs)

func _mat(color: Color, metallic := 0.62, roughness := 0.52) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat

func _build_visual() -> void:
    var body := MeshInstance3D.new()
    body.position = Vector3(0,0.62,0)
    var body_mesh := BoxMesh.new()
    body_mesh.size = Vector3(1.15,1.25,1.55)
    body_mesh.material = _mat(Color("30373d"))
    body.mesh = body_mesh
    add_child(body)

    var barrel := MeshInstance3D.new()
    barrel.position = Vector3(0,0.94,-1.0)
    barrel.rotation_degrees.x = 90.0
    var barrel_mesh := CylinderMesh.new()
    barrel_mesh.top_radius = 0.12
    barrel_mesh.bottom_radius = 0.18
    barrel_mesh.height = 1.45
    barrel_mesh.radial_segments = 8
    barrel_mesh.material = _mat(Color("1b2024"),0.8,0.38)
    barrel.mesh = barrel_mesh
    add_child(barrel)

    status_light = OmniLight3D.new()
    status_light.position = Vector3(0.38,1.1,-0.5)
    status_light.light_color = Color("bd8d55")
    status_light.light_energy = 0.5
    status_light.omni_range = 2.8
    add_child(status_light)

    cable = MeshInstance3D.new()
    cable_mesh = CylinderMesh.new()
    cable_mesh.top_radius = 0.034
    cable_mesh.bottom_radius = 0.034
    cable_mesh.height = 0.1
    cable_mesh.radial_segments = 7
    cable_mesh.material = _mat(Color("171a1d"),0.76,0.48)
    cable.mesh = cable_mesh
    cable.visible = false
    add_child(cable)

func fire() -> bool:
    if is_instance_valid(target):
        return true
    var from := global_position + global_basis * Vector3(0,0.94,-1.7)
    var to := from + (-global_basis.z) * 82.0
    var query := PhysicsRayQueryParameters3D.create(from,to)
    query.exclude = [self]
    var hit: Dictionary = get_world_3d().direct_space_state.intersect_ray(query)
    if hit.is_empty():
        _flash_status(Color("9d493e"))
        return false
    var collider := hit.get("collider") as ColossusWalker
    if not is_instance_valid(collider):
        _flash_status(Color("9d493e"))
        return false
    target = collider
    var world_hit: Vector3 = hit.get("position", target.global_position)
    target_local_point = target.to_local(world_hit)
    var count := int(target.get_meta("harpoon_count",0)) + 1
    target.set_meta("harpoon_count",count)
    _recompute_target_speed(target,count)
    enabled = true
    cable.visible = true
    _flash_status(Color("67d0a1"))
    return true

func _detach() -> void:
    if is_instance_valid(target):
        var count := maxi(0,int(target.get_meta("harpoon_count",1)) - 1)
        target.set_meta("harpoon_count",count)
        _recompute_target_speed(target,count)
    target = null
    enabled = false
    if is_instance_valid(cable):
        cable.visible = false
    _flash_status(Color("bd8d55"))

func _recompute_target_speed(colossus: ColossusWalker, count: int) -> void:
    if not is_instance_valid(colossus):
        return
    var leg_factor := 0.34 if bool(colossus.destroyed.get("legs",false)) else 1.0
    colossus.move_speed = 1.05 * leg_factor / (1.0 + float(count) * 0.28)

func _update_cable() -> void:
    if not is_instance_valid(target) or not is_instance_valid(cable):
        return
    var a := global_position + global_basis * Vector3(0,0.94,-1.7)
    var b := target.to_global(target_local_point)
    var delta := b-a
    if delta.length() < 0.1:
        return
    cable.visible = true
    cable.global_position = (a+b)*0.5
    cable_mesh.height = delta.length()
    cable.quaternion = Quaternion(Vector3.UP,delta.normalized())

func _flash_status(color: Color) -> void:
    if not is_instance_valid(status_light):
        return
    status_light.light_color = color
    status_light.light_energy = 0.85

func set_signal(value: bool) -> void:
    if value:
        fire()
    elif is_instance_valid(target):
        _detach()

func get_prompt_text() -> String:
    if is_instance_valid(target):
        return "[E] Harpoon Anchor  TENSION  |  SHIFT+E release"
    return "[E] Harpoon Anchor  |  fire at Colossus in line of sight"

func interact(_player: Node) -> void:
    if Input.is_key_pressed(KEY_SHIFT):
        _detach()
    else:
        fire()

func _exit_tree() -> void:
    if is_instance_valid(target):
        _detach()
