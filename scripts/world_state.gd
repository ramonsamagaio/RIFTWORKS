class_name RiftWorldState
extends Node

const STATE_PATH := "user://riftworks_world_state.json"
const WORLD_SEED := 731942
const SAVE_DEBOUNCE := 0.35

var removed_ids: Dictionary = {}
var dirty := false
var save_timer := 0.0

func _ready() -> void:
    add_to_group("world_state")
    _load_state()

func _process(delta: float) -> void:
    if not dirty:
        return
    save_timer -= delta
    if save_timer <= 0.0:
        _save_state()

func is_removed(persistent_id: String) -> bool:
    return not persistent_id.is_empty() and removed_ids.has(persistent_id)

func mark_removed(persistent_id: String) -> void:
    if persistent_id.is_empty() or removed_ids.has(persistent_id):
        return
    removed_ids[persistent_id] = true
    dirty = true
    save_timer = SAVE_DEBOUNCE

func get_world_seed() -> int:
    return WORLD_SEED

func _load_state() -> void:
    if not FileAccess.file_exists(STATE_PATH):
        return
    var file := FileAccess.open(STATE_PATH, FileAccess.READ)
    if file == null:
        return
    var parsed: Variant = JSON.parse_string(file.get_as_text())
    file.close()
    if not parsed is Dictionary:
        return
    var data := parsed as Dictionary
    if int(data.get("world_seed", WORLD_SEED)) != WORLD_SEED:
        return
    var stored: Variant = data.get("removed_ids", {})
    if stored is Dictionary:
        removed_ids = stored as Dictionary

func _save_state() -> void:
    var file := FileAccess.open(STATE_PATH, FileAccess.WRITE)
    if file == null:
        return
    file.store_string(JSON.stringify({
        "version": 1,
        "world_seed": WORLD_SEED,
        "removed_ids": removed_ids
    }))
    file.close()
    dirty = false

func _exit_tree() -> void:
    if dirty:
        _save_state()
