extends Node3D

const WORLD_SIZE := 220.0
var player: CharacterBody3D
var camera: Camera3D
var yaw := 0.0
var pitch := -0.22
var battery := 100.0
var scrap := 0
var power := 0.0
var flashlight_on := true
var hud: Label
var rng := RandomNumberGenerator.new()

func _ready():
    rng.seed = 1337
    _make_environment()
    _make_ground()
    _make_city()
    _make_underground()
    _make_colossus()
    _make_player()
    _make_hud()
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _process(delta):
    if flashlight_on:
        battery = maxf(0.0, battery - delta * 0.55)
        if battery <= 0.0: _set_flashlight(false)
    hud.text = "RIFTWORKS  //  NIGHT PROTOTYPE\nSCRAP %03d   BATTERY %03d%%   GRID %.1f kW\nWASD move  |  mouse look  |  F flashlight  |  E scavenge  |  B deploy light  |  ESC mouse" % [scrap, int(battery), power]

func _unhandled_input(event):
    if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        yaw -= event.relative.x * 0.0025
        pitch = clampf(pitch - event.relative.y * 0.0025, -1.0, 0.45)
    if event is InputEventKey and event.pressed:
        if event.keycode == KEY_ESCAPE:
            Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
        elif event.keycode == KEY_F: _set_flashlight(not flashlight_on)
        elif event.keycode == KEY_E: _scavenge()
        elif event.keycode == KEY_B: _deploy_light()

func _physics_process(delta):
    if not player: return
    var input := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
    var basis := Basis(Vector3.UP, yaw)
    var dir := (basis * Vector3(input.x, 0, input.y)).normalized()
    player.velocity.x = dir.x * 7.0
    player.velocity.z = dir.z * 7.0
    if not player.is_on_floor(): player.velocity.y -= 24.0 * delta
    player.move_and_slide()
    var target := player.global_position + Vector3(0, 1.4, 0)
    var orbit := Basis(Vector3.UP, yaw) * Basis(Vector3.RIGHT, pitch)
    camera.global_position = target + orbit * Vector3(0, 1.1, 5.8)
    camera.look_at(target)

func _make_environment():
    var world_env := WorldEnvironment.new()
    var env := Environment.new()
    env.background_mode = Environment.BG_COLOR
    env.background_color = Color("050710")
    env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
    env.ambient_light_color = Color("11172b")
    env.ambient_light_energy = 0.22
    env.fog_enabled = true
    env.fog_light_color = Color("10172a")
    env.fog_density = 0.012
    env.fog_height = 2.0
    env.fog_height_density = 0.08
    world_env.environment = env
    add_child(world_env)
    var moon := DirectionalLight3D.new()
    moon.light_color = Color("778bbd")
    moon.light_energy = 0.18
    moon.rotation_degrees = Vector3(-55, -35, 0)
    moon.shadow_enabled = true
    add_child(moon)

func _box(pos: Vector3, size: Vector3, color: Color, parent: Node = self) -> StaticBody3D:
    var body := StaticBody3D.new()
    body.position = pos
    var mesh := MeshInstance3D.new()
    var cube := BoxMesh.new(); cube.size = size
    var mat := StandardMaterial3D.new(); mat.albedo_color = color; mat.roughness = 0.9
    cube.material = mat; mesh.mesh = cube; body.add_child(mesh)
    var shape := CollisionShape3D.new(); var box := BoxShape3D.new(); box.size = size; shape.shape = box; body.add_child(shape)
    parent.add_child(body)
    return body

func _make_ground():
    _box(Vector3(0,-0.6,0), Vector3(WORLD_SIZE,1,WORLD_SIZE), Color("11151b"))
    _box(Vector3(0,0.01,0), Vector3(12,0.05,WORLD_SIZE), Color("191b20"))
    _box(Vector3(55,0.02,0), Vector3(8,0.06,WORLD_SIZE), Color("17191d"))

func _make_city():
    for z in range(-90, 91, 18):
        for side in [-1, 1]:
            if rng.randf() < 0.18: continue
            var x := side * rng.randf_range(13.0, 38.0)
            var floors := rng.randi_range(1,4)
            var w := rng.randf_range(8,14); var d := rng.randf_range(8,14); var h := floors * rng.randf_range(3.2,4.2)
            _box(Vector3(x,h/2.0,z+rng.randf_range(-3,3)), Vector3(w,h,d), Color(0.08+rng.randf()*0.04,0.085,0.09))
    for i in 26:
        var lamp := OmniLight3D.new(); lamp.position = Vector3(rng.randf_range(-5,5),3.2,rng.randf_range(-100,100)); lamp.omni_range = 8; lamp.light_energy = 0.0; add_child(lamp)

func _make_underground():
    var entrance := _box(Vector3(-58,-1.5,-25), Vector3(18,1,18), Color("15191d"))
    for i in 8:
        _box(Vector3(-58,-4.0-i*3.2,-25-i*7.0), Vector3(12,0.7,8), Color("20252b"))
    _box(Vector3(-58,-29,-82), Vector3(34,1,48), Color("171b20"))
    _box(Vector3(-75,-22,-82), Vector3(1,16,48), Color("232831"))
    _box(Vector3(-41,-22,-82), Vector3(1,16,48), Color("232831"))
    var eerie := OmniLight3D.new(); eerie.position = Vector3(-58,-20,-96); eerie.light_color = Color("6249ff"); eerie.light_energy = 2.0; eerie.omni_range = 22; add_child(eerie)

func _make_colossus():
    var c := Node3D.new(); c.position = Vector3(70,0,-75); add_child(c)
    var mat := Color("191824")
    _box(Vector3(-5,14,0), Vector3(3,28,3), mat, c)
    _box(Vector3(5,14,0), Vector3(3,28,3), mat, c)
    _box(Vector3(0,31,0), Vector3(14,10,7), mat, c)
    _box(Vector3(0,39,0), Vector3(7,7,6), Color("24202e"), c)
    var eye := OmniLight3D.new(); eye.position = Vector3(0,40,-3); eye.light_color = Color("d8b7ff"); eye.light_energy = 5; eye.omni_range = 28; c.add_child(eye)

func _make_player():
    player = CharacterBody3D.new(); player.position = Vector3(0,1,35); add_child(player)
    var collision := CollisionShape3D.new(); var capsule := CapsuleShape3D.new(); capsule.radius=.45; capsule.height=1.8; collision.shape=capsule; player.add_child(collision)
    var mesh := MeshInstance3D.new(); var cap := CapsuleMesh.new(); cap.radius=.45; cap.height=1.8; var mat:=StandardMaterial3D.new(); mat.albedo_color=Color("606b72"); cap.material=mat; mesh.mesh=cap; player.add_child(mesh)
    var light := SpotLight3D.new(); light.name="Flashlight"; light.position=Vector3(0,1.1,-0.25); light.rotation_degrees.x=-90; light.light_energy=8; light.spot_range=24; light.spot_angle=28; light.shadow_enabled=true; player.add_child(light)
    camera = Camera3D.new(); camera.current=true; add_child(camera)

func _make_hud():
    var layer := CanvasLayer.new(); add_child(layer)
    hud=Label.new(); hud.position=Vector2(20,18); hud.add_theme_font_size_override("font_size",18); layer.add_child(hud)
    var cross:=Label.new(); cross.text="+"; cross.position=Vector2(635,350); cross.add_theme_font_size_override("font_size",24); layer.add_child(cross)

func _set_flashlight(value: bool):
    flashlight_on=value and battery>0
    if player and player.has_node("Flashlight"): player.get_node("Flashlight").visible=flashlight_on

func _scavenge():
    scrap += rng.randi_range(1,4)
    if rng.randf()<0.25: battery=minf(100.0,battery+12.0)

func _deploy_light():
    if scrap < 5: return
    scrap -= 5
    var stand:=Node3D.new(); stand.position=player.global_position-Vector3(0,0.4,0); add_child(stand)
    _box(Vector3.ZERO,Vector3(.4,2.6,.4),Color("333941"),stand)
    var lamp:=OmniLight3D.new(); lamp.position=Vector3(0,1.6,0); lamp.light_color=Color("ffd7a1"); lamp.light_energy=4; lamp.omni_range=15; lamp.shadow_enabled=true; stand.add_child(lamp)
    power += 0.4
