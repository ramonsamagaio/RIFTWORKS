class_name PowerGridSystem
extends Node3D

const SIM_TIME_SCALE := 60.0

var devices: Array[PowerDevice] = []
var links: Dictionary = {}
var dirty: bool = true
var total_generation_kw: float = 0.0
var total_consumption_kw: float = 0.0
var total_storage_kwh: float = 0.0
var total_charge_kwh: float = 0.0

func register_device(device: PowerDevice) -> void:
    if devices.has(device):
        return
    devices.append(device)
    links[device.get_instance_id()] = []
    device.grid = self
    dirty = true

func unregister_device(device: PowerDevice) -> void:
    if not devices.has(device):
        return
    var id: int = device.get_instance_id()
    devices.erase(device)
    links.erase(id)
    for key: Variant in links.keys():
        var linked_list: Array = links[key] as Array
        linked_list.erase(id)
    dirty = true

func connect_devices(a: PowerDevice, b: PowerDevice) -> void:
    if not is_instance_valid(a) or not is_instance_valid(b) or a == b:
        return
    register_device(a)
    register_device(b)
    var a_id: int = a.get_instance_id()
    var b_id: int = b.get_instance_id()
    var a_links: Array = links[a_id] as Array
    var b_links: Array = links[b_id] as Array
    if b_id in a_links:
        return
    a_links.append(b_id)
    b_links.append(a_id)
    _make_cable_visual(a, b)
    dirty = true

func auto_connect(device: PowerDevice, max_distance: float = 14.0) -> PowerDevice:
    var best: PowerDevice
    var best_distance: float = max_distance
    for candidate: PowerDevice in devices:
        if candidate == device or not is_instance_valid(candidate):
            continue
        var distance: float = device.global_position.distance_to(candidate.global_position)
        if distance < best_distance:
            best_distance = distance
            best = candidate
    register_device(device)
    if is_instance_valid(best):
        connect_devices(device, best)
    return best

func force_recompute() -> void:
    dirty = true

func _process(delta: float) -> void:
    _cleanup()
    _balance_all(delta)

func _cleanup() -> void:
    for i: int in range(devices.size() - 1, -1, -1):
        if not is_instance_valid(devices[i]):
            devices.remove_at(i)
            dirty = true

func _balance_all(delta: float) -> void:
    total_generation_kw = 0.0
    total_consumption_kw = 0.0
    total_storage_kwh = 0.0
    total_charge_kwh = 0.0

    var visited: Dictionary = {}
    for device: PowerDevice in devices:
        if not is_instance_valid(device):
            continue
        var id: int = device.get_instance_id()
        if visited.has(id):
            continue
        var component: Array[PowerDevice] = _collect_component(device, visited)
        _balance_component(component, delta)

    for device: PowerDevice in devices:
        if not is_instance_valid(device):
            continue
        if device.kind == PowerDevice.Kind.GENERATOR and device.enabled:
            total_generation_kw += device.generation_kw
        elif device.kind == PowerDevice.Kind.CONSUMER and device.enabled:
            total_consumption_kw += device.consumption_kw
        elif device.kind == PowerDevice.Kind.BATTERY:
            total_storage_kwh += device.capacity_kwh
            total_charge_kwh += device.charge_kwh

func _collect_component(start: PowerDevice, visited: Dictionary) -> Array[PowerDevice]:
    var result: Array[PowerDevice] = []
    var queue: Array[PowerDevice] = [start]
    while not queue.is_empty():
        var current: PowerDevice = queue.pop_front() as PowerDevice
        if not is_instance_valid(current):
            continue
        var id: int = current.get_instance_id()
        if visited.has(id):
            continue
        visited[id] = true
        result.append(current)
        var linked_ids: Array = links.get(id, []) as Array
        for linked_id_value: Variant in linked_ids:
            var linked_id: int = int(linked_id_value)
            var linked: PowerDevice = _find_device_by_id(linked_id)
            if is_instance_valid(linked):
                queue.append(linked)
    return result

func _find_device_by_id(id: int) -> PowerDevice:
    for device: PowerDevice in devices:
        if is_instance_valid(device) and device.get_instance_id() == id:
            return device
    return null

func _balance_component(component: Array[PowerDevice], delta: float) -> void:
    var generators: Array[PowerDevice] = []
    var batteries: Array[PowerDevice] = []
    var consumers: Array[PowerDevice] = []

    for device: PowerDevice in component:
        match device.kind:
            PowerDevice.Kind.GENERATOR:
                if device.enabled:
                    generators.append(device)
            PowerDevice.Kind.BATTERY:
                if device.enabled:
                    batteries.append(device)
            PowerDevice.Kind.CONSUMER:
                if device.enabled:
                    consumers.append(device)
                else:
                    device.set_powered(false)

    consumers.sort_custom(func(a: PowerDevice, b: PowerDevice) -> bool: return a.priority < b.priority)

    var generation: float = 0.0
    for generator: PowerDevice in generators:
        generation += generator.generation_kw

    var dt_hours: float = delta * SIM_TIME_SCALE / 3600.0
    var remaining_generation: float = generation

    for consumer: PowerDevice in consumers:
        if remaining_generation >= consumer.consumption_kw:
            remaining_generation -= consumer.consumption_kw
            consumer.set_powered(true)
            continue

        var deficit_kw: float = consumer.consumption_kw - remaining_generation
        var required_kwh: float = deficit_kw * dt_hours
        var available_kwh: float = 0.0
        for battery: PowerDevice in batteries:
            available_kwh += battery.charge_kwh

        if available_kwh + 0.00001 >= required_kwh:
            _drain_batteries(batteries, required_kwh)
            remaining_generation = 0.0
            consumer.set_powered(true)
        else:
            if available_kwh > 0.0:
                _drain_batteries(batteries, available_kwh)
            remaining_generation = 0.0
            consumer.set_powered(false)

    if remaining_generation > 0.0 and not batteries.is_empty():
        var surplus_kwh: float = remaining_generation * dt_hours
        _charge_batteries(batteries, surplus_kwh)

func _drain_batteries(batteries: Array[PowerDevice], amount_kwh: float) -> void:
    var remaining: float = amount_kwh
    for battery: PowerDevice in batteries:
        if remaining <= 0.0:
            break
        var take: float = minf(battery.charge_kwh, remaining)
        battery.charge_kwh -= take
        remaining -= take
        battery._refresh_visual_state()

func _charge_batteries(batteries: Array[PowerDevice], amount_kwh: float) -> void:
    var remaining: float = amount_kwh
    for battery: PowerDevice in batteries:
        if remaining <= 0.0:
            break
        var room: float = maxf(0.0, battery.capacity_kwh - battery.charge_kwh)
        var amount_to_add: float = minf(room, remaining)
        battery.charge_kwh += amount_to_add
        remaining -= amount_to_add
        battery._refresh_visual_state()

func _make_cable_visual(a: PowerDevice, b: PowerDevice) -> void:
    var delta_position: Vector3 = b.global_position - a.global_position
    var cable_length: float = delta_position.length()
    if cable_length <= 0.01:
        return
    var cable := MeshInstance3D.new()
    cable.name = "PowerCable"
    cable.position = (a.global_position + b.global_position) * 0.5 + Vector3(0, 0.45, 0)
    var cylinder := CylinderMesh.new()
    cylinder.top_radius = 0.035
    cylinder.bottom_radius = 0.035
    cylinder.height = cable_length
    cylinder.radial_segments = 6
    var mat := StandardMaterial3D.new()
    mat.albedo_color = Color("16181b")
    mat.roughness = 0.82
    cylinder.material = mat
    cable.mesh = cylinder
    cable.quaternion = Quaternion(Vector3.UP, delta_position.normalized())
    add_child(cable)

func get_summary() -> String:
    var storage_percent: float = 0.0
    if total_storage_kwh > 0.001:
        storage_percent = total_charge_kwh / total_storage_kwh * 100.0
    return "GRID %.1f kW GEN | %.1f kW LOAD | %.0f%% STORAGE" % [total_generation_kw, total_consumption_kw, storage_percent]
