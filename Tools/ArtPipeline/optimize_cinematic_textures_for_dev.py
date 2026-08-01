from __future__ import annotations

import unreal

ROOTS = (
    "/Game/Art/CC0/PolyHaven",
    "/Game/Art/Generated/Character",
)


def log(message: str) -> None:
    unreal.log(f"[ARASH Texture Budget] {message}")


def main() -> None:
    changed = 0
    skipped = 0

    for root in ROOTS:
        for asset_path in unreal.EditorAssetLibrary.list_assets(
            root,
            recursive=True,
            include_folder=False,
        ):
            asset = unreal.load_asset(asset_path)
            if asset is None or not isinstance(asset, unreal.Texture):
                continue

            try:
                target_size = 1024 if isinstance(asset, unreal.TextureCube) else 2048
                current_size = int(asset.get_editor_property("max_texture_size"))
                if current_size != target_size:
                    asset.set_editor_property("max_texture_size", target_size)
                    changed += 1
                    log(f"Limited {asset_path} to {target_size}px")

                try:
                    asset.set_editor_property("never_stream", False)
                except Exception:
                    pass

                unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
            except Exception as exc:
                skipped += 1
                unreal.log_warning(f"[ARASH Texture Budget] Skipped {asset_path}: {exc}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Complete: {changed} textures changed, {skipped} skipped. Restart Unreal before testing.")


if __name__ == "__main__":
    main()
