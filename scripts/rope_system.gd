class_name RopeSystem
extends Node3D

const SEGMENT_LENGTH := 0.62
const MAX_SEGMENTS := 18

var player: RiftPlayer
var rope_counter := 0

func _ready() -> void:
    await get_tree().process_frame
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer

func _unhandled_input(event: InputEvent) -> void:
    if not is_instance_valid(player) or not event is InputEventKey or not event.pressed or event.echo:
        return
    if event.keycode == KEY_F6:
        _connect_nearest_with_rope()

func _aim_position() -> Vector3:
    var from := player.camera.global_position
    var to := from + (-player.camera.global_basis.z) * 9.0
    var query := PhysicsRayQueryParameters3D.create(from,to)
    query.exclude = [player]
    var hit: Dictionary = player.get_world_3d().direct_space_state.intersect_ray(query)
    return hit.get("position", to) as Vector3 if not hit.is_empty() else to

func _candidate_bodies(pos: Vector3, max_distance: float) -> Array[RigidBody3D]:
    var result: Array[RigidBody3D] = []
    var seen: Dictionary = {}
    var groups: Array[String] = ["assembly_parts","salvage"]
    for group_name in groups:
        for raw_node: Node in get_tree().get_nodes_in_group(group_name):
            var body := raw_node as RigidBody3D
            if not is_instance_valid(body):
                continue
            if body is SalvageProp and (body as SalvageProp).mass_class <= 0:
                continue
            var id := body.get_instance_id()
            if seen.has(id):
                continue
            seen[id] = true
            if body.global_position.distance_to(pos) <= max_distance:
                result.append(body)
    result.sort_custom(func(a: RigidBody3D, b: RigidBody3D) -> bool:
        return a.global_position.distance_squared_to(pos) < b.global_position.distance_squared_to(pos)
    )
    if result.size() > 2:
        result.resize(2)
    return result

func _connect_nearest_with_rope() -> void:
    if int(player.components.get("cable",0)) < 1:
        return
    var bodies := _candidate_bodies(_aim_position(), 7.0)
    if bodies.size() < 2:
        return
    var a := bodies[0]
    var b := bodies[1]
    var distance := a.global_position.distance_to(b.global_position)
    if distance < 0.8 or distance > 12.0:
        return
    player.components["cable"] = int(player.components.get("cable",0)) - 1
    _build_rope(a,b)

func _build_rope(a: RigidBody3D, b: RigidBody3D) -> void:
    rope_counter += 1
    var root := Node3D.new()
    root.name = "PhysicalRope_%03d" % rope_counter
    add_child(root)

    var from := a.global_position
    var to := b.global_position
    var distance := from.distance_to(to)
    var segment_count := clampi(ceili(distance / SEGMENT_LENGTH), 2, MAX_SEGMENTS)
    var previous: RigidBody3D = a

    for i in range(segment_count):
        var t := float(i + 1) / float(segment_count + 1)
        var pos := from.lerp(to,t) + Vector3.DOWN * sin(t * PI) * minf(0.65,distance * 0.06)
        var segment := _make_segment(root,pos,i)
        _pin(root,previous,segment,(previous.global_position + segment.global_position) * 0.5)
        previous = segment

    _pin(root,previous,b,(previous.global_position + b.global_position) * 0.5)

func _make_segment(parent: Node3D, pos: Vector3, index: int) -> RigidBody3D:
    var body := RigidBody3D.new()
    body.name = "RopeSegment_%02d" % index
    body.mass = 0.38
    body.global_position = pos
    body.linear_damp = 0.35
    body.angular_damp = 0.7
    body.continuous_cd = true

    var cs := CollisionShape3D.new()
    var shape := SphereShape3D.new()
    shape.radius = 0.085
    cs.shape = shape
    body.add_child(cs)

    var visual := MeshInstance3D.new()
    var mesh := SphereMesh.new()
    mesh.radius = 0.09
    mesh.height = 0.18
    mesh.radial_segments = 6
    mesh.rings = 4
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("272b2e")
    mat.roughness = 0.92
    mesh.material = mat
    visual.mesh = mesh
    visual.gi_mode = GeometryInstance3D.GI_MODE_DYNAMIC
    body.add_child(visual)
    parent.add_child(body)
    return body

func _pin(parent: Node3D, a: RigidBody3D, b: RigidBody3D, position: Vector3) -> void:
    var joint := PinJoint3D.new()
    joint.global_position = position
    parent.add_child(joint)
    joint.node_a = joint.get_path_to(a)
    joint.node_b = joint.get_path_to(b)
