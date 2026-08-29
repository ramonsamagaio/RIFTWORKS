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
    import riftworks_encounter_staging
    import riftworks_instance_polish
    import riftworks_salvage_fabrication
    import riftworks_logistics_machines
    import riftworks_creatures
    import riftworks_breach_progression
    import riftworks_network_temperature

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
    riftworks_encounter_staging.apply_all()
    riftworks_instance_polish.apply_all()
    riftworks_salvage_fabrication.apply_all()
    riftworks_logistics_machines.apply_all()
    riftworks_creatures.apply_all()
    riftworks_breach_progression.apply_all()
    riftworks_network_temperature.apply_all()
except Exception as exc:
    unreal.log_error(
        f"[RIFTWORKS] Automatic setup could not finish: {exc}. "
        "After C++ compiles, run Content/Python passes in setup -> polish -> extras -> vertical_slice -> scene_dressing -> accessibility -> weathering -> landmarks -> encounter_staging -> instance_polish -> salvage_fabrication -> logistics_machines -> creatures -> breach_progression -> network_temperature order."
    )
