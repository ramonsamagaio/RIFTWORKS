import unreal

try:
    import riftworks_setup
    import riftworks_extras

    required = [
        "/Game/Riftworks/Blueprints/BP_RiftPlayer",
        "/Game/Riftworks/Blueprints/BP_RiftHumanoid",
        "/Game/Riftworks/Maps/L_RiftworksBootstrap",
    ]
    if not all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in required):
        unreal.log("[RIFTWORKS] First Unreal open detected. Running Blueprint/asset migration bootstrap...")
        riftworks_setup.setup_all(True)
    else:
        unreal.log("[RIFTWORKS] Core Unreal content already exists; refreshing Blueprint extras only.")

    riftworks_extras.apply_all()
except Exception as exc:
    unreal.log_error(
        f"[RIFTWORKS] Automatic setup could not finish: {exc}. "
        "After C++ compiles, run Content/Python/riftworks_setup.py and riftworks_extras.py from Tools > Execute Python Script."
    )
