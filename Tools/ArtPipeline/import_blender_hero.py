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
BASIC_MATERIAL = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"


def log(message: str) -> None:
    unreal.log(f"[ARASH Hero Import] {message}")


def reset_materials(mesh: unreal.StaticMesh) -> None:
    fallback = unreal.load_asset(BASIC_MATERIAL)
    if fallback is None:
        raise RuntimeError(f"Fallback material not found: {BASIC_MATERIAL}")

    static_materials = list(mesh.get_editor_property("static_materials"))
    for index in range(len(static_materials)):
        mesh.set_material(index, fallback)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)


def remove_support_assets() -> None:
    removed = 0
    for path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=False, include_folder=False):
        asset = unreal.load_asset(path)
        if asset is None or isinstance(asset, unreal.StaticMesh):
            continue
        if unreal.EditorAssetLibrary.delete_asset(path):
            removed += 1
            log(f"Removed support asset {path}")
    log(f"Removed {removed} generated support assets.")


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
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.convert_scene = True
    options.static_mesh_import_data.convert_scene_unit = True
    task.options = options

    log(f"Importing modular hero kit: {SOURCE_FBX}")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    meshes: list[unreal.StaticMesh] = []
    for path in task.imported_object_paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            reset_materials(asset)
            meshes.append(asset)
            log(f"Imported mesh {path}")

    if len(meshes) < 8:
        raise RuntimeError(f"Expected modular hero meshes, but Unreal imported only {len(meshes)}.")

    remove_support_assets()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Hero import complete: {len(meshes)} static meshes. Restart Unreal after rebuilding C++.")


if __name__ == "__main__":
    main()
