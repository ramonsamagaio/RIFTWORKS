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
    riftworks_vertical_slice.apply_all()
    riftworks_scene_dressing.apply_all()
    riftworks_accessibility_pass.apply_all()
    riftworks_weathering.apply_all()
    riftworks_landmarks.apply_all()
    riftworks_beauty_pass.apply_all()
    riftworks_encounter_staging.apply_all()
    riftworks_salvage_fabrication.apply_all()
    riftworks_logistics_machines.apply_all()
    riftworks_creatures.apply_all()
    riftworks_breach_progression.apply_all()
    riftworks_network_temperature.apply_all()
    riftworks_road_graph.apply_all()
    riftworks_engineering_proving_ground.apply_all()
    riftworks_production_player.apply_all()

    # Always last. All generated actors are normalized after every art/gameplay pass,
    # so legacy pivot/Rotator assumptions cannot re-corrupt the playable map.
    riftworks_instance_polish.apply_all()

    # Source-level guardrail. Any future direct positional Unreal Rotator call
    # fails loudly instead of silently rotating world geometry on the wrong axes.
    riftworks_source_audit.apply_all()
except Exception as exc:
    unreal.log_error(
        f"[RIFTWORKS] Automatic setup could not finish: {exc}. "
        "After C++ compiles, run Content/Python passes in setup -> polish -> extras -> vertical_slice -> scene_dressing -> accessibility -> weathering -> landmarks -> beauty_pass -> encounter_staging -> salvage_fabrication -> logistics_machines -> creatures -> breach_progression -> network_temperature -> road_graph -> engineering_proving_ground -> production_player -> FINAL instance_polish -> source_audit order."
    )
