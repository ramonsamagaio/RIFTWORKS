import unreal

try:
    import riftworks_setup
    import riftworks_extras
    import riftworks_polish
    import riftworks_vertical_slice
    import riftworks_scene_dressing
    import riftworks_accessibility_pass
    import riftworks_weathering
    import riftworks_landmarks
    import riftworks_beauty_pass
    import riftworks_lived_in_pass
    import riftworks_atmosphere_refine
    import riftworks_city_map
    import riftworks_encounter_staging
    import riftworks_instance_polish
    import riftworks_salvage_fabrication
    import riftworks_logistics_machines
    import riftworks_creatures
    import riftworks_breach_progression
    import riftworks_network_temperature
    import riftworks_road_graph
    import riftworks_engineering_proving_ground
    import riftworks_production_player
    import riftworks_runtime_fixes
    import riftworks_art_layout_sanitize
    import riftworks_cosmetic_collision
    import riftworks_source_audit

    required = [
        "/Game/Riftworks/Blueprints/BP_RiftPlayer",
        "/Game/Riftworks/Blueprints/BP_RiftHumanoid",
        "/Game/Riftworks/Maps/L_RiftworksBootstrap",
    ]
    first_open = not all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in required)

    if first_open:
        unreal.log("[RIFTWORKS] First Unreal open detected. Running full Blueprint/asset migration bootstrap...")
        riftworks_setup.setup_all(True)
    else:
        unreal.log("[RIFTWORKS] Existing Unreal content detected. Refreshing mannequin/animation references...")
        riftworks_setup.setup_all(False)

    riftworks_polish.apply_all()
    riftworks_extras.apply_all()

    # Keep the proven underground / Breach route as the structural foundation.
    # The modular city pass later removes only the obsolete surface greybox.
    riftworks_vertical_slice.apply_all()
    riftworks_scene_dressing.apply_all()
    riftworks_accessibility_pass.apply_all()
    riftworks_weathering.apply_all()
    riftworks_landmarks.apply_all()
    riftworks_beauty_pass.apply_all()
    riftworks_lived_in_pass.apply_all()
    riftworks_atmosphere_refine.apply_all()

    # If the committed CityBuildings source pack is present, it becomes the
    # authoritative surface world: real streets, modular enterable shells,
    # subway headhouse, skyline and placed loot containers.
    city_ready = False
    if riftworks_city_map.has_source_pack():
        city_ready = bool(riftworks_city_map.apply_all())
        if city_ready:
            unreal.log("[RIFTWORKS] Modular CityBuildings map is authoritative for the surface district.")

    riftworks_encounter_staging.apply_all()
    riftworks_salvage_fabrication.apply_all()
    riftworks_logistics_machines.apply_all()
    riftworks_creatures.apply_all()
    riftworks_breach_progression.apply_all()
    riftworks_network_temperature.apply_all()

    # The old outskirts graph is useful only as a future procedural test. Do not
    # lay another road network on top of the authored CityBuildings district.
    if not city_ready:
        riftworks_road_graph.apply_all()

    riftworks_engineering_proving_ground.apply_all()
    riftworks_production_player.apply_all()

    # FINAL gameplay layer. This deliberately runs after encounter staging and
    # production-player creation so stale humanoids are replaced by the same
    # Skeleton-native / bounds-grounded strategy that already works on Colossus,
    # and the stable flashlight + visual inventory pawn becomes GameMode default.
    riftworks_runtime_fixes.apply_all()

    # Resolve functional actors against the authored interior layout.
    riftworks_art_layout_sanitize.apply_all()

    # Old micro-detail layers are cosmetic. CityKit structural meshes keep their
    # imported collision, while decorative legacy details never block the player.
    riftworks_cosmetic_collision.apply_all()

    # All runtime actors get one final grounding/material sanity pass.
    riftworks_instance_polish.apply_all()

    # Source-level guardrail. Any future direct positional Unreal Rotator call
    # fails loudly instead of silently rotating world geometry on the wrong axes.
    riftworks_source_audit.apply_all()
except Exception as exc:
    unreal.log_error(
        f"[RIFTWORKS] Automatic setup could not finish: {exc}. "
        "After C++ compiles, run setup -> polish -> extras -> vertical_slice -> accessibility -> city_map -> encounter_staging -> gameplay systems -> production_player -> runtime_fixes -> final polish -> source_audit."
    )
