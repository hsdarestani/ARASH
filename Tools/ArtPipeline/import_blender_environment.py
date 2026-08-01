from __future__ import annotations

import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_FBX = os.path.join(
    PROJECT_DIR,
    "ArtSource",
    "Generated",
    "Blender",
    "Environment",
    "ARASH_EnvironmentKit.fbx",
)
DESTINATION = "/Game/Art/Generated/Environment"


def log(message: str) -> None:
    unreal.log(f"[ARASH Blender Import] {message}")


def main() -> None:
    if not os.path.isfile(SOURCE_FBX):
        raise RuntimeError(
            "Generated FBX not found. Run Tools/Blender/run_environment.ps1 first: "
            + SOURCE_FBX
        )

    task = unreal.AssetImportTask()
    task.filename = SOURCE_FBX
    task.destination_path = DESTINATION
    task.automated = True
    task.replace_existing = True
    task.save = False

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = True
    options.create_physics_asset = False
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.auto_generate_collision = True
    options.static_mesh_import_data.convert_scene = True
    options.static_mesh_import_data.convert_scene_unit = True
    task.options = options

    log(f"Importing {SOURCE_FBX}")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = list(task.imported_object_paths)
    if not imported_paths:
        raise RuntimeError("Unreal did not report any imported environment assets.")

    for asset_path in imported_paths:
        log(f"Imported {asset_path}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Environment import complete. Restart Unreal, rebuild ARASH, then Play NewMap.")


if __name__ == "__main__":
    main()
