extends SceneTree

func _initialize() -> void:
    var failed: Array[String] = []
    var checked: int = 0
    var script_files: PackedStringArray = DirAccess.get_files_at("res://scripts")
    script_files.sort()

    for file_name: String in script_files:
        if not file_name.ends_with(".gd"):
            continue
        checked += 1
        var path: String = "res://scripts/%s" % file_name
        var resource: Resource = ResourceLoader.load(path, "Script", ResourceLoader.CACHE_MODE_IGNORE)
        if resource == null:
            failed.append(path)

    if not failed.is_empty():
        push_error("Strict compile failed for: %s" % ", ".join(failed))
        quit(1)
        return

    print("Strict GDScript compile gate passed: %d scripts" % checked)
    quit(0)
