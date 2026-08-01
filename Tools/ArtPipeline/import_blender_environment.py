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
BASIC_MATERIAL = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"


def log(message: str) -> None:
    unreal.log(f"[ARASH Blender Import] {message}")


def reset_mesh_material_references(mesh: unreal.StaticMesh) -> None:
    """Break references to FBX-created 4K material/texture duplicates.

    Runtime C++ assigns the final PBR and accent materials by material-slot name.
    Keeping only a tiny engine material on the saved mesh prevents the generated
    content directory from loading a second copy of every Poly Haven texture.
    """
    fallback = unreal.load_asset(BASIC_MATERIAL)
    if fallback is None:
        raise RuntimeError(f"Engine fallback material was not found: {BASIC_MATERIAL}")

    static_materials = list(mesh.get_editor_property("static_materials"))
    for index in range(len(static_materials)):
        mesh.set_material(index, fallback)

    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)


def remove_duplicate_support_assets() -> None:
    removed = 0
    for asset_path in unreal.EditorAssetLibrary.list_assets(
        DESTINATION,
        recursive=False,
        include_folder=False,
    ):
        asset = unreal.load_asset(asset_path)
        if asset is None or isinstance(asset, unreal.StaticMesh):
            continue

        if unreal.EditorAssetLibrary.delete_asset(asset_path):
            removed += 1
            log(f"Removed duplicated FBX support asset {asset_path}")

    log(f"Removed {removed} duplicated material/texture assets.")


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
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.static_mesh_import_data.combine_meshes = False
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.auto_generate_collision = True
    options.static_mesh_import_data.convert_scene = True
    options.static_mesh_import_data.convert_scene_unit = True
    task.options = options

    log(f"Importing mesh-only environment kit: {SOURCE_FBX}")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = list(task.imported_object_paths)
    imported_meshes: list[unreal.StaticMesh] = []
    for asset_path in imported_paths:
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            imported_meshes.append(asset)
            reset_mesh_material_references(asset)
            log(f"Imported mesh {asset_path}")

    if not imported_meshes:
        raise RuntimeError("Unreal did not report any imported environment meshes.")

    remove_duplicate_support_assets()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(
        "Environment import complete. Only meshes are retained; runtime assigns the final materials."
    )


if __name__ == "__main__":
    main()
