import unreal

try:
    import riftworks_setup

    required = [
        "/Game/Riftworks/Blueprints/BP_RiftPlayer",
        "/Game/Riftworks/Blueprints/BP_RiftHumanoid",
        "/Game/Riftworks/Maps/L_RiftworksBootstrap",
    ]
    if not all(unreal.EditorAssetLibrary.does_asset_exist(path) for path in required):
        unreal.log("[RIFTWORKS] First Unreal open detected. Running Blueprint/asset migration bootstrap...")
        riftworks_setup.setup_all(True)
    else:
        unreal.log("[RIFTWORKS] Unreal content bootstrap already exists. Run riftworks_setup.py manually to refresh it.")
except Exception as exc:
    unreal.log_error(f"[RIFTWORKS] Automatic setup could not finish: {exc}. After C++ compiles, run Content/Python/riftworks_setup.py from Tools > Execute Python Script.")
