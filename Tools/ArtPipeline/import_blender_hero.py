from __future__ import annotations

import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_FBX = os.path.join(
    PROJECT_DIR,
    "ArtSource",
    "Generated",
    "Blender",
    "Character",
    "ARASH_HeroKit.fbx",
)
DESTINATION = "/Game/Art/Generated/Character"


def log(message: str) -> None:
    unreal.log(f"[ARASH Hero Import] {message}")


def main() -> None:
    if not os.path.isfile(SOURCE_FBX):
        raise RuntimeError(
            "Generated hero FBX not found. Run Tools/Blender/run_hero.ps1 first: " + SOURCE_FBX
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
    options.import_textures = False
    options.create_physics_asset = False
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.convert_scene = True
    options.static_mesh_import_data.convert_scene_unit = True
    task.options = options

    log(f"Importing modular hero kit with lightweight flat-color materials: {SOURCE_FBX}")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    meshes: list[unreal.StaticMesh] = []
    materials = 0
    for path in task.imported_object_paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)
            log(f"Imported mesh {path}")
        elif isinstance(asset, unreal.MaterialInterface):
            materials += 1

    if len(meshes) < 8:
        raise RuntimeError(f"Expected modular hero meshes, but Unreal imported only {len(meshes)}.")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(
        f"Hero import complete: {len(meshes)} static meshes and {materials} lightweight materials. "
        "Restart Unreal after rebuilding C++."
    )


if __name__ == "__main__":
    main()
