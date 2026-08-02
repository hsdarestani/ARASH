from __future__ import annotations

import json
import py_compile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = REPO_ROOT / "Tools" / "Art" / "cc0_vertical_slice_manifest.json"
IMPORTER_PATH = REPO_ROOT / "Content" / "Python" / "import_cc0_vertical_slice.py"
REQUIRED_ROLES = {"FloorTile", "FloorMedallion", "Wall", "Pillar", "Brazier"}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"[ARASH CC0 Validate] {message}")


def main() -> None:
    require(MANIFEST_PATH.is_file(), f"Missing manifest: {MANIFEST_PATH}")
    require(IMPORTER_PATH.is_file(), f"Missing importer: {IMPORTER_PATH}")

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    require(manifest.get("source", {}).get("commit"), "Source commit must be pinned")
    require(manifest.get("source", {}).get("license") == "CC0-1.0", "Expected CC0-1.0 license")
    require(manifest.get("destination_path", "").startswith("/Game/"), "Invalid Unreal destination path")

    assets = manifest.get("assets", [])
    require(bool(assets), "Manifest does not contain assets")

    roles = [asset.get("role") for asset in assets]
    sources = [asset.get("source") for asset in assets]
    destinations = [asset.get("destination") for asset in assets]

    require(REQUIRED_ROLES.issubset(set(roles)), "Required environment roles are incomplete")
    require(len(sources) == len(set(sources)), "Duplicate source FBX entries")
    require(len(destinations) == len(set(destinations)), "Duplicate Unreal destination names")

    for asset in assets:
        source = asset.get("source", "")
        destination = asset.get("destination", "")
        require(source.lower().endswith(".fbx"), f"Non-FBX source: {source}")
        require("/" not in source and "\\" not in source and ".." not in source, f"Unsafe source path: {source}")
        require(destination.startswith("SM_ARASH_"), f"Unexpected destination naming: {destination}")
        require(bool(asset.get("material_slot")), f"Missing material slot for {destination}")

    py_compile.compile(str(IMPORTER_PATH), doraise=True)
    print(f"[ARASH CC0 Validate] OK: {len(assets)} pinned assets, importer syntax valid.")


if __name__ == "__main__":
    main()
