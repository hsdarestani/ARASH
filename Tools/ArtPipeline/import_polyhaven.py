import json
import os
import re

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_ROOT = os.path.join(PROJECT_DIR, "ArtSource", "PolyHaven")
MANIFEST_PATH = os.path.join(SOURCE_ROOT, "ARASH_CC0_MANIFEST.json")
DEST_ROOT = "/Game/Art/CC0/PolyHaven"


def log(message: str) -> None:
    unreal.log(f"[ARASH ArtPipeline] {message}")


def safe_asset_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_]", "_", value)
    return re.sub(r"_+", "_", cleaned).strip("_")


def import_file(filename: str, destination_path: str):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = destination_path
    task.automated = True
    task.replace_existing = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported_paths = list(task.imported_object_paths)
    if not imported_paths:
        raise RuntimeError(f"Unreal did not import {filename}")

    return unreal.EditorAssetLibrary.load_asset(imported_paths[0])


def prepare_texture(texture, filename: str) -> None:
    lower = filename.lower()

    if "_nor_dx_" in lower:
        texture.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP
        )
        texture.set_editor_property("srgb", False)
    elif any(token in lower for token in ("_arm_", "_ao_", "_rough_", "_metal_", "_disp_")):
        texture.set_editor_property("srgb", False)

    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def create_texture_sample(material, texture, x: int, y: int):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, x, y
    )
    node.texture = texture
    return node


def create_or_rebuild_material(asset_id: str, textures: dict, destination_path: str):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_name = f"M_{safe_asset_name(asset_id)}"
    material_path = f"{destination_path}/{material_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        material = unreal.EditorAssetLibrary.load_asset(material_path)
    else:
        factory = unreal.MaterialFactoryNew()
        material = asset_tools.create_asset(
            material_name, destination_path, unreal.Material, factory
        )

    if not material:
        raise RuntimeError(f"Could not create material {material_path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    diff = textures.get("diff")
    normal = textures.get("normal")
    arm = textures.get("arm")
    ao = textures.get("ao")
    rough = textures.get("rough")
    metal = textures.get("metal")

    if diff:
        node = create_texture_sample(material, diff, -720, -180)
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
        )

    if normal:
        node = create_texture_sample(material, normal, -720, 40)
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "RGB", unreal.MaterialProperty.MP_NORMAL
        )

    if arm:
        node = create_texture_sample(material, arm, -720, 270)
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "G", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "B", unreal.MaterialProperty.MP_METALLIC
        )
    else:
        if ao:
            node = create_texture_sample(material, ao, -720, 220)
            unreal.MaterialEditingLibrary.connect_material_property(
                node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
            )
        if rough:
            node = create_texture_sample(material, rough, -720, 340)
            unreal.MaterialEditingLibrary.connect_material_property(
                node, "R", unreal.MaterialProperty.MP_ROUGHNESS
            )
        if metal:
            node = create_texture_sample(material, metal, -720, 460)
            unreal.MaterialEditingLibrary.connect_material_property(
                node, "R", unreal.MaterialProperty.MP_METALLIC
            )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    log(f"Built {material_path}")
    return material


def classify_texture(filename: str):
    lower = filename.lower()
    if "_diff_" in lower:
        return "diff"
    if "_nor_dx_" in lower:
        return "normal"
    if "_arm_" in lower:
        return "arm"
    if "_ao_" in lower:
        return "ao"
    if "_rough_" in lower:
        return "rough"
    if "_metal_" in lower:
        return "metal"
    if "_disp_" in lower:
        return "disp"
    return None


def main() -> None:
    if not os.path.isfile(MANIFEST_PATH):
        raise RuntimeError(
            "Missing ArtSource/PolyHaven/ARASH_CC0_MANIFEST.json. "
            "Run Tools/ArtPipeline/fetch_polyhaven.ps1 first."
        )

    with open(MANIFEST_PATH, "r", encoding="utf-8-sig") as handle:
        manifest = json.load(handle)

    log("Importing CC0 source art. Powered by Poly Haven.")

    for item in manifest:
        asset_id = item["id"]
        asset_type = item["type"]
        source_folder = os.path.join(SOURCE_ROOT, asset_id)
        destination = f"{DEST_ROOT}/{safe_asset_name(asset_id)}"

        if asset_type == "hdri":
            for filename in item.get("files", []):
                full_path = os.path.join(source_folder, filename)
                if os.path.isfile(full_path):
                    import_file(full_path, f"{DEST_ROOT}/HDRI")
                    log(f"Imported HDRI {filename}")
            continue

        textures = {}
        for filename in item.get("files", []):
            role = classify_texture(filename)
            if not role:
                continue

            full_path = os.path.join(source_folder, filename)
            if not os.path.isfile(full_path):
                log(f"Skipping missing source file: {full_path}")
                continue

            imported = import_file(full_path, destination)
            if isinstance(imported, unreal.Texture):
                prepare_texture(imported, filename)
                textures[role] = imported

        if "diff" not in textures:
            log(f"WARNING: {asset_id} has no diffuse map; material skipped")
            continue

        create_or_rebuild_material(asset_id, textures, destination)

    log("Import complete. Save All, then commit Content/Art/CC0 with Git LFS.")


if __name__ == "__main__":
    main()
