from __future__ import annotations

import json
from pathlib import Path

import unreal


LOG_PREFIX = "[ARASH CC0 Import]"


def log(message: str) -> None:
    unreal.log(f"{LOG_PREFIX} {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"{LOG_PREFIX} {message}")


def fail(message: str) -> None:
    unreal.log_error(f"{LOG_PREFIX} {message}")
    raise RuntimeError(message)


def load_manifest(repo_root: Path) -> dict:
    manifest_path = repo_root / "Tools" / "Art" / "cc0_vertical_slice_manifest.json"
    if not manifest_path.is_file():
        fail(f"Manifest does not exist: {manifest_path}")
    return json.loads(manifest_path.read_text(encoding="utf-8-sig"))


def make_fbx_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", True)

    for property_name, value in (
        ("convert_scene", True),
        ("convert_scene_unit", True),
        ("force_front_x_axis", False),
        ("import_uniform_scale", 1.0),
    ):
        try:
            static_data.set_editor_property(property_name, value)
        except Exception as exc:  # Unreal versions expose slightly different FBX properties.
            warn(f"Could not set FBX option '{property_name}': {exc}")

    return options


def import_static_mesh(source_file: Path, destination_path: str, destination_name: str):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", make_fbx_options())

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    object_path = f"{destination_path}/{destination_name}"
    mesh = unreal.EditorAssetLibrary.load_asset(object_path)
    if not mesh:
        imported_paths = list(task.get_editor_property("imported_object_paths") or [])
        fail(
            f"Import did not create {object_path}. "
            f"Imported paths: {', '.join(str(path) for path in imported_paths)}"
        )

    if not isinstance(mesh, unreal.StaticMesh):
        fail(f"Imported asset is not a StaticMesh: {object_path}")

    return mesh, object_path


def set_material_slot_names(mesh: unreal.StaticMesh, material_slot: str) -> None:
    try:
        static_materials = list(mesh.get_editor_property("static_materials"))
    except Exception as exc:
        warn(f"Could not read material slots for {mesh.get_name()}: {exc}")
        return

    if not static_materials:
        warn(f"No imported material slots found for {mesh.get_name()}")
        return

    changed = False
    for index, static_material in enumerate(static_materials):
        slot_name = material_slot if index == 0 else f"{material_slot}_{index + 1}"
        try:
            static_material.set_editor_property("material_slot_name", unreal.Name(slot_name))
            static_material.set_editor_property("imported_material_slot_name", unreal.Name(slot_name))
            changed = True
        except Exception as exc:
            warn(f"Could not rename slot {index} on {mesh.get_name()}: {exc}")

    if changed:
        try:
            mesh.set_editor_property("static_materials", static_materials)
        except Exception as exc:
            warn(f"Could not write material slots for {mesh.get_name()}: {exc}")


def main() -> None:
    repo_root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    manifest = load_manifest(repo_root)
    staging_root = repo_root / Path(manifest["staging_root"])
    source_fbx_root = staging_root / "Assets" / "fbx"
    destination_path = manifest["destination_path"]

    if not source_fbx_root.is_dir():
        fail(
            f"Staged FBX directory does not exist: {source_fbx_root}. "
            "Run Tools/Art/install_cc0_vertical_slice.ps1 first."
        )

    log(f"Importing CC0 vertical-slice kit into {destination_path}")
    imported = []
    missing_required = []

    for definition in manifest["assets"]:
        source_file = source_fbx_root / definition["source"]
        if not source_file.is_file():
            if definition.get("required", False):
                missing_required.append(str(source_file))
            else:
                warn(f"Skipping optional missing source: {source_file}")
            continue

        log(f"{definition['role']}: {source_file.name} -> {definition['destination']}")
        mesh, object_path = import_static_mesh(
            source_file,
            destination_path,
            definition["destination"],
        )
        set_material_slot_names(mesh, definition["material_slot"])
        unreal.EditorAssetLibrary.save_asset(object_path, only_if_is_dirty=False)
        imported.append(object_path)

    if missing_required:
        fail("Required staged assets are missing: " + ", ".join(missing_required))

    required_paths = [
        f"{destination_path}/{item['destination']}"
        for item in manifest["assets"]
        if item.get("required", False)
    ]
    verification_failures = [
        asset_path
        for asset_path in required_paths
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path)
    ]
    if verification_failures:
        fail("Required Unreal assets were not created: " + ", ".join(verification_failures))

    unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)
    log(f"Ready. Imported {len(imported)} static meshes.")
    log("Open /Game/Maps/PrototypeArena/NewMap and press Play to see the CC0 arena.")


if __name__ == "__main__":
    main()
