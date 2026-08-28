class_name AdaptiveHUD
extends CanvasLayer

var player: RiftPlayer
var grid: PowerGridSystem
var status_label: Label
var prompt_label: Label
var crosshair: Label
var inventory_panel: PanelContainer
var inventory_label: Label
var build_panel: PanelContainer
var build_label: Label
var inventory_open := false
var build_help_open := true

func _ready() -> void:
    layer = 20
    await get_tree().process_frame
    await get_tree().process_frame
    player = get_tree().get_first_node_in_group("player") as RiftPlayer
    var runtime_grid := get_parent().get_node_or_null("PowerGrid")
    if runtime_grid is PowerGridSystem:
        grid = runtime_grid as PowerGridSystem
    var old_hud := get_parent().get_node_or_null("HUD")
    if is_instance_valid(old_hud):
        old_hud.visible = false
    _build_ui()

func _unhandled_input(event: InputEvent) -> void:
    if not event is InputEventKey or not event.pressed or event.echo:
        return
    match event.keycode:
        KEY_TAB:
            inventory_open = not inventory_open
            inventory_panel.visible = inventory_open
            if inventory_open:
                Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
            else:
                Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
        KEY_Q:
            build_help_open = not build_help_open
            build_panel.visible = build_help_open

func _process(_delta: float) -> void:
    if not is_instance_valid(player):
        return
    var grid_text := "GRID OFFLINE"
    if is_instance_valid(grid):
        grid_text = grid.get_summary()
    status_label.text = "%s\n%s" % [player.get_hud_status(), grid_text]
    prompt_label.text = player.interaction_prompt
    _refresh_inventory()

func _panel_style(alpha := 0.82) -> StyleBoxFlat:
    var style := StyleBoxFlat.new()
    style.bg_color = Color(0.008, 0.012, 0.019, alpha)
    style.border_color = Color(0.20, 0.27, 0.33, 0.72)
    style.set_border_width_all(1)
    style.corner_radius_top_left = 5
    style.corner_radius_top_right = 5
    style.corner_radius_bottom_left = 5
    style.corner_radius_bottom_right = 5
    style.content_margin_left = 14
    style.content_margin_right = 14
    style.content_margin_top = 10
    style.content_margin_bottom = 10
    return style

func _build_ui() -> void:
    var root := Control.new()
    root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    root.mouse_filter = Control.MOUSE_FILTER_IGNORE
    add_child(root)

    var status_panel := PanelContainer.new()
    status_panel.add_theme_stylebox_override("panel", _panel_style(0.72))
    status_panel.position = Vector2(22, 22)
    status_panel.custom_minimum_size = Vector2(650, 72)
    root.add_child(status_panel)
    status_label = Label.new()
    status_label.add_theme_font_size_override("font_size", 16)
    status_label.add_theme_color_override("font_color", Color("d8e4eb"))
    status_panel.add_child(status_label)

    crosshair = Label.new()
    crosshair.text = "+"
    crosshair.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    crosshair.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
    crosshair.add_theme_font_size_override("font_size", 22)
    crosshair.add_theme_color_override("font_color", Color(0.92, 0.96, 1.0, 0.72))
    crosshair.set_anchors_preset(Control.PRESET_CENTER)
    crosshair.position = Vector2(-16, -18)
    crosshair.size = Vector2(32, 36)
    root.add_child(crosshair)

    prompt_label = Label.new()
    prompt_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
    prompt_label.position = Vector2(-280, -158)
    prompt_label.size = Vector2(560, 42)
    prompt_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    prompt_label.add_theme_font_size_override("font_size", 18)
    prompt_label.add_theme_color_override("font_color", Color("f4ddb0"))
    root.add_child(prompt_label)

    build_panel = PanelContainer.new()
    build_panel.add_theme_stylebox_override("panel", _panel_style(0.70))
    build_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
    build_panel.position = Vector2(-382, 22)
    build_panel.size = Vector2(360, 184)
    root.add_child(build_panel)
    build_label = Label.new()
    build_label.text = "FIELD ENGINEERING   [Q hide]\nB floodlight   G generator   T battery\n1 platform   2 beam   3 wheel   4 motor wheel\n5 anchor   X motors   O save blueprint   P rebuild\n6 button   7 sensor   8 signal lamp   9 alarm\nK connect signal   0 repulsion   N attraction   M luminance"
    build_label.add_theme_font_size_override("font_size", 14)
    build_label.add_theme_color_override("font_color", Color("b9c9d3"))
    build_panel.add_child(build_label)

    inventory_panel = PanelContainer.new()
    inventory_panel.add_theme_stylebox_override("panel", _panel_style(0.94))
    inventory_panel.set_anchors_preset(Control.PRESET_CENTER)
    inventory_panel.position = Vector2(-280, -220)
    inventory_panel.size = Vector2(560, 440)
    inventory_panel.visible = false
    inventory_panel.mouse_filter = Control.MOUSE_FILTER_STOP
    root.add_child(inventory_panel)
    inventory_label = Label.new()
    inventory_label.add_theme_font_size_override("font_size", 18)
    inventory_label.add_theme_color_override("font_color", Color("dce6ec"))
    inventory_panel.add_child(inventory_label)

    var hint := Label.new()
    hint.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
    hint.position = Vector2(22, -48)
    hint.text = "TAB inventory   Q engineering   F flashlight   E interact   SHIFT sprint"
    hint.add_theme_font_size_override("font_size", 14)
    hint.add_theme_color_override("font_color", Color(0.65, 0.72, 0.77, 0.82))
    root.add_child(hint)

func _refresh_inventory() -> void:
    if not is_instance_valid(inventory_label):
        return
    inventory_label.text = "RIFTWORKS // FIELD INVENTORY\n\nSCRAP                         %d\nBATTERY CELLS                 %d\nCABLE COILS                   %d\nCONTROL ELECTRONICS           %d\nINDUSTRIAL MOTORS             %d\nFUEL                          %d\nBREACH CORES                  %d\nCOLOSSUS BIOELECTRIC CORES    %d\nCARAPACE                      %d\n\nHeavy recovered parts must be physically carried to a claimed base\nbefore they are secured into inventory.\n\n[TAB] close" % [
        player.scrap,
        int(player.components.get("battery_cell", 0)),
        int(player.components.get("cable", 0)),
        int(player.components.get("electronics", 0)),
        int(player.components.get("motor", 0)),
        int(player.components.get("fuel", 0)),
        int(player.components.get("breach_core", 0)),
        int(player.components.get("bioelectric_core", 0)),
        int(player.components.get("carapace", 0))
    ]
