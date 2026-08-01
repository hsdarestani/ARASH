from __future__ import annotations

import math
import random
from pathlib import Path

import bpy
from mathutils import Vector


REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATED_ROOT = REPO_ROOT / "ArtSource" / "Generated" / "Blender"
EXPORT_DIR = GENERATED_ROOT / "Environment"
FBX_PATH = EXPORT_DIR / "ARASH_EnvironmentKit.fbx"
BLEND_PATH = GENERATED_ROOT / "ARASH_EnvironmentKit.blend"
POLYHAVEN_ROOT = REPO_ROOT / "ArtSource" / "PolyHaven"

ASSET_COLLECTION = "ARASH_ENVIRONMENT_KIT"


def log(message: str) -> None:
    print(f"[ARASH Blender] {message}", flush=True)


def reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
    ):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)

    for collection in list(bpy.data.collections):
        if collection.name != "Collection":
            bpy.data.collections.remove(collection)

    root = bpy.context.scene.collection
    default = bpy.data.collections.get("Collection")
    if default:
        default.name = ASSET_COLLECTION
    else:
        default = bpy.data.collections.new(ASSET_COLLECTION)
        root.children.link(default)

    bpy.context.view_layer.active_layer_collection = bpy.context.view_layer.layer_collection.children[default.name]

    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_EEVEE_NEXT"


def clear_selection() -> None:
    bpy.ops.object.select_all(action="DESELECT")


def apply_bevel(obj: bpy.types.Object, width: float = 0.035, segments: int = 3) -> None:
    if width <= 0:
        return
    modifier = obj.modifiers.new(name="ARASH_Bevel", type="BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def assign_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    if not obj.data.materials:
        obj.data.materials.append(material)
    else:
        obj.data.materials[0] = material


def make_principled(
    name: str,
    base_color: tuple[float, float, float, float],
    metallic: float = 0.0,
    roughness: float = 0.55,
    emission: tuple[float, float, float, float] | None = None,
    emission_strength: float = 0.0,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.inputs["Base Color"].default_value = base_color
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    if emission is not None:
        shader.inputs["Emission Color"].default_value = emission
        shader.inputs["Emission Strength"].default_value = emission_strength

    material.node_tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def first_matching_file(folder: Path, tokens: tuple[str, ...]) -> Path | None:
    if not folder.exists():
        return None
    candidates = sorted(p for p in folder.iterdir() if p.is_file())
    for token in tokens:
        token_lower = token.lower()
        for candidate in candidates:
            if token_lower in candidate.name.lower() and candidate.suffix.lower() in {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".exr"}:
                return candidate
    return None


def make_polyhaven_material(
    name: str,
    asset_id: str,
    fallback_color: tuple[float, float, float, float],
    metallic: float = 0.0,
    roughness: float = 0.65,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    links = material.node_tree.links

    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.inputs["Base Color"].default_value = fallback_color
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])

    folder = POLYHAVEN_ROOT / asset_id
    diffuse = first_matching_file(folder, ("_diff_", "diffuse", "albedo", "basecolor"))
    normal = first_matching_file(folder, ("_nor_dx_", "normal_dx", "normal"))
    rough = first_matching_file(folder, ("_rough_", "roughness"))
    metal = first_matching_file(folder, ("_metal_", "metallic"))
    arm = first_matching_file(folder, ("_arm_",))

    def image_node(path: Path, label: str, non_color: bool = False):
        image = bpy.data.images.load(str(path), check_existing=True)
        if non_color:
            image.colorspace_settings.name = "Non-Color"
        node = nodes.new("ShaderNodeTexImage")
        node.image = image
        node.label = label
        return node

    if diffuse:
        node = image_node(diffuse, "Base Color")
        links.new(node.outputs["Color"], shader.inputs["Base Color"])

    if normal:
        node = image_node(normal, "Normal", non_color=True)
        normal_map = nodes.new("ShaderNodeNormalMap")
        normal_map.space = "TANGENT"
        links.new(node.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])

    if arm:
        node = image_node(arm, "ARM", non_color=True)
        separate = nodes.new("ShaderNodeSeparateColor")
        links.new(node.outputs["Color"], separate.inputs["Color"])
        links.new(separate.outputs["Red"], shader.inputs["Ambient Occlusion"])
        links.new(separate.outputs["Green"], shader.inputs["Roughness"])
        links.new(separate.outputs["Blue"], shader.inputs["Metallic"])
    else:
        if rough:
            node = image_node(rough, "Roughness", non_color=True)
            links.new(node.outputs["Color"], shader.inputs["Roughness"])
        if metal:
            node = image_node(metal, "Metallic", non_color=True)
            links.new(node.outputs["Color"], shader.inputs["Metallic"])

    return material


def box(
    name: str,
    dimensions: tuple[float, float, float],
    location: tuple[float, float, float],
    material: bpy.types.Material,
    bevel: float = 0.035,
    rotation_z: float = 0.0,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=(0.0, 0.0, rotation_z))
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    apply_bevel(obj, bevel)
    assign_material(obj, material)
    return obj


def cylinder(
    name: str,
    radius: float,
    depth: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
    vertices: int = 48,
    bevel: float = 0.025,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    obj = bpy.context.object
    obj.name = name
    apply_bevel(obj, bevel)
    assign_material(obj, material)
    return obj


def torus(
    name: str,
    major_radius: float,
    minor_radius: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=64,
        minor_segments=16,
        location=location,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, material)
    return obj


def sphere(
    name: str,
    radius: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
    scale_z: float = 1.0,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48, ring_count=24, radius=radius, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale.z = scale_z
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, material)
    return obj


def star_prism(
    name: str,
    outer_radius: float,
    inner_radius: float,
    points: int,
    depth: float,
    z: float,
    material: bpy.types.Material,
) -> bpy.types.Object:
    verts: list[tuple[float, float, float]] = []
    faces: list[tuple[int, ...]] = []
    count = points * 2
    for layer_z in (-depth / 2.0, depth / 2.0):
        for index in range(count):
            angle = math.pi * index / points
            radius = outer_radius if index % 2 == 0 else inner_radius
            verts.append((math.cos(angle) * radius, math.sin(angle) * radius, z + layer_z))

    faces.append(tuple(range(count - 1, -1, -1)))
    faces.append(tuple(range(count, count * 2)))
    for index in range(count):
        nxt = (index + 1) % count
        faces.append((index, nxt, count + nxt, count + index))

    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, material)
    apply_bevel(obj, 0.025)
    return obj


def curve_strip(
    name: str,
    points: list[tuple[float, float, float]],
    bevel_depth: float,
    material: bpy.types.Material,
) -> bpy.types.Object:
    curve_data = bpy.data.curves.new(name=f"{name}_Curve", type="CURVE")
    curve_data.dimensions = "3D"
    curve_data.bevel_depth = bevel_depth
    curve_data.bevel_resolution = 3
    spline = curve_data.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for point, coordinates in zip(spline.points, points):
        point.co = (*coordinates, 1.0)
    obj = bpy.data.objects.new(name, curve_data)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, material)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return bpy.context.object


def join_parts(parts: list[bpy.types.Object], name: str) -> bpy.types.Object:
    clear_selection()
    valid_parts = [part for part in parts if part and part.name in bpy.context.view_layer.objects]
    for part in valid_parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = valid_parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.context.scene.cursor.location = (0.0, 0.0, 0.0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    clear_selection()
    return obj


def add_asset_tag(obj: bpy.types.Object) -> None:
    obj["ARASH_Asset"] = True
    obj["unreal_nanite"] = True


def create_floor_tile(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Floor_Base", (4.0, 4.0, 0.16), (0, 0, 0.08), mats["floor"], 0.045),
        box("Floor_Inner", (3.56, 3.56, 0.035), (0, 0, 0.177), mats["stone"], 0.018),
    ]
    for sign in (-1, 1):
        parts.append(box("Floor_Gold_X", (3.72, 0.055, 0.025), (0, sign * 1.82, 0.205), mats["gold"], 0.01))
        parts.append(box("Floor_Gold_Y", (0.055, 3.72, 0.025), (sign * 1.82, 0, 0.205), mats["gold"], 0.01))
    parts += [
        box("Floor_Diag_A", (2.75, 0.045, 0.022), (0, 0, 0.208), mats["turquoise"], 0.008, math.radians(45)),
        box("Floor_Diag_B", (2.75, 0.045, 0.022), (0, 0, 0.209), mats["turquoise"], 0.008, math.radians(-45)),
    ]
    obj = join_parts(parts, "SM_ARASH_FloorTile_A")
    add_asset_tag(obj)
    return obj


def create_floor_medallion(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        cylinder("Medallion_Base", 3.25, 0.16, (0, 0, 0.08), mats["floor"], 96, 0.03),
        torus("Medallion_GoldOuter", 2.95, 0.085, (0, 0, 0.19), mats["gold"]),
        torus("Medallion_Turquoise", 2.55, 0.075, (0, 0, 0.20), mats["turquoise"]),
        star_prism("Medallion_Star", 2.2, 1.05, 8, 0.05, 0.20, mats["gold"]),
        cylinder("Medallion_Core", 0.72, 0.07, (0, 0, 0.225), mats["turquoise"], 64, 0.02),
        torus("Medallion_CoreRing", 0.72, 0.065, (0, 0, 0.265), mats["gold"]),
    ]
    obj = join_parts(parts, "SM_ARASH_FloorMedallion_A")
    add_asset_tag(obj)
    return obj


def create_wall(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Wall_Main", (4.0, 0.72, 3.15), (0, 0, 1.575), mats["wall"], 0.055),
        box("Wall_Base", (4.2, 0.92, 0.34), (0, 0, 0.17), mats["stone"], 0.035),
        box("Wall_Cornice_A", (4.22, 0.98, 0.25), (0, 0, 3.14), mats["stone"], 0.035),
        box("Wall_Cornice_B", (4.38, 1.04, 0.18), (0, 0, 3.36), mats["stone"], 0.035),
        box("Wall_Panel", (3.35, 0.10, 1.72), (0, -0.405, 1.72), mats["stone"], 0.03),
        box("Wall_GoldTrim", (3.62, 0.055, 0.065), (0, -0.47, 2.78), mats["gold"], 0.012),
        box("Wall_TurqTrim", (3.45, 0.052, 0.055), (0, -0.475, 2.54), mats["turquoise"], 0.012),
    ]
    for x in (-1.72, 1.72):
        parts += [
            box("Wall_Pilaster", (0.25, 0.17, 2.55), (x, -0.45, 1.55), mats["stone"], 0.025),
            box("Wall_PilasterCap", (0.42, 0.22, 0.20), (x, -0.45, 2.92), mats["gold"], 0.025),
        ]

    parts.append(cylinder("Relief_Sun", 0.32, 0.10, (0, -0.52, 1.88), mats["gold"], 48, 0.018))
    for side in (-1, 1):
        for row in range(4):
            y = 1.88 - row * 0.18
            start = 0.25 * side
            end = (0.75 + row * 0.24) * side
            parts.append(
                curve_strip(
                    f"Relief_Wing_{side}_{row}",
                    [(start, -0.54, y), (end * 0.65, -0.58, y + 0.08), (end, -0.55, y - 0.02)],
                    0.035,
                    mats["gold"],
                )
            )
    parts.append(curve_strip("Relief_Tail", [(0, -0.55, 1.56), (-0.22, -0.56, 1.20), (0, -0.55, 0.96), (0.22, -0.56, 1.20)], 0.04, mats["gold"]))

    obj = join_parts(parts, "SM_ARASH_Wall_A")
    add_asset_tag(obj)
    return obj


def create_pillar(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Pillar_BaseA", (1.55, 1.55, 0.30), (0, 0, 0.15), mats["stone"], 0.04),
        box("Pillar_BaseB", (1.25, 1.25, 0.28), (0, 0, 0.44), mats["turquoise"], 0.035),
        box("Pillar_BaseC", (1.05, 1.05, 0.24), (0, 0, 0.70), mats["gold"], 0.035),
        cylinder("Pillar_Shaft", 0.48, 4.10, (0, 0, 2.87), mats["wall"], 32, 0.035),
        torus("Pillar_RingA", 0.54, 0.075, (0, 0, 1.00), mats["gold"]),
        torus("Pillar_RingB", 0.54, 0.075, (0, 0, 4.72), mats["gold"]),
        box("Pillar_CapitalA", (1.18, 1.18, 0.26), (0, 0, 5.02), mats["turquoise"], 0.035),
        box("Pillar_CapitalB", (1.55, 1.05, 0.30), (0, 0, 5.31), mats["stone"], 0.045),
    ]
    for side in (-1, 1):
        parts.append(sphere(f"Capital_Bull_{side}", 0.42, (side * 0.47, 0, 5.58), mats["gold"], 0.68))
        parts.append(cylinder(f"Capital_Horn_{side}", 0.10, 0.62, (side * 0.75, -0.12, 5.73), mats["gold"], 24, 0.018))
    obj = join_parts(parts, "SM_ARASH_Pillar_A")
    add_asset_tag(obj)
    return obj


def create_brazier(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Brazier_BaseA", (1.25, 1.25, 0.25), (0, 0, 0.125), mats["stone"], 0.04),
        box("Brazier_BaseB", (0.95, 0.95, 0.25), (0, 0, 0.36), mats["turquoise"], 0.035),
        cylinder("Brazier_Stem", 0.30, 0.72, (0, 0, 0.84), mats["metal"], 40, 0.025),
        cylinder("Brazier_Bowl", 0.76, 0.26, (0, 0, 1.33), mats["metal"], 64, 0.035),
        torus("Brazier_Rim", 0.76, 0.085, (0, 0, 1.47), mats["gold"]),
        cylinder("Brazier_Ember", 0.58, 0.08, (0, 0, 1.50), mats["emissive"], 64, 0.01),
    ]
    for angle in range(0, 360, 45):
        radians = math.radians(angle)
        parts.append(
            box(
                "Brazier_Ornament",
                (0.10, 0.24, 0.34),
                (math.cos(radians) * 0.58, math.sin(radians) * 0.58, 1.26),
                mats["gold"],
                0.018,
                radians,
            )
        )
    obj = join_parts(parts, "SM_ARASH_Brazier_A")
    add_asset_tag(obj)
    return obj


def create_banner(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    width = 1.5
    height = 3.4
    cols = 10
    rows = 22
    verts = []
    faces = []
    for row in range(rows):
        z = height * (1 - row / (rows - 1))
        for col in range(cols):
            x = -width / 2 + width * col / (cols - 1)
            y = 0.05 * math.sin(row * 0.55 + col * 0.35) + 0.035 * math.sin(col * 1.1)
            verts.append((x, y, z))
    for row in range(rows - 1):
        for col in range(cols - 1):
            a = row * cols + col
            b = a + 1
            c = a + cols + 1
            d = a + cols
            faces.append((a, b, c, d))
    mesh = bpy.data.meshes.new("Banner_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    cloth = bpy.data.objects.new("Banner_Cloth", mesh)
    bpy.context.collection.objects.link(cloth)
    assign_material(cloth, mats["crimson"])
    solidify = cloth.modifiers.new("Banner_Solidify", "SOLIDIFY")
    solidify.thickness = 0.018
    bpy.context.view_layer.objects.active = cloth
    bpy.ops.object.modifier_apply(modifier=solidify.name)

    parts = [
        cloth,
        cylinder("Banner_Rod", 0.055, 1.95, (0, 0, height + 0.12), mats["gold"], 24, 0.012),
        star_prism("Banner_Emblem", 0.38, 0.16, 8, 0.035, 1.72, mats["gold"]),
    ]
    parts[1].rotation_euler = (0.0, math.radians(90), 0.0)
    bpy.context.view_layer.objects.active = parts[1]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    obj = join_parts(parts, "SM_ARASH_Banner_A")
    add_asset_tag(obj)
    return obj


def create_broken_column(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Broken_Base", (1.35, 1.35, 0.28), (0, 0, 0.14), mats["stone"], 0.04),
        cylinder("Broken_Shaft", 0.43, 2.7, (0.52, 0.08, 0.72), mats["wall"], 32, 0.035),
        torus("Broken_Ring", 0.50, 0.075, (1.39, 0.08, 1.36), mats["gold"]),
    ]
    shaft = parts[1]
    shaft.rotation_euler = (0.0, math.radians(72), math.radians(12))
    parts[2].rotation_euler = shaft.rotation_euler
    for obj in (shaft, parts[2]):
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    obj = join_parts(parts, "SM_ARASH_BrokenColumn_A")
    add_asset_tag(obj)
    return obj


def create_rubble(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    random.seed(1701)
    parts = []
    for index in range(18):
        x = random.uniform(-1.8, 1.8)
        y = random.uniform(-1.5, 1.5)
        z = random.uniform(0.08, 0.20)
        size = random.uniform(0.18, 0.52)
        obj = box(
            f"Rubble_{index}",
            (size * random.uniform(0.8, 1.5), size, size * random.uniform(0.55, 1.05)),
            (x, y, z),
            mats["stone"] if index % 3 else mats["wall"],
            0.04,
            random.uniform(-math.pi, math.pi),
        )
        obj.rotation_euler.x = random.uniform(-0.25, 0.25)
        obj.rotation_euler.y = random.uniform(-0.25, 0.25)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
        parts.append(obj)
    result = join_parts(parts, "SM_ARASH_Rubble_A")
    add_asset_tag(result)
    return result


def create_gate(mats: dict[str, bpy.types.Material]) -> bpy.types.Object:
    parts = [
        box("Gate_Left", (1.0, 1.15, 4.8), (-2.35, 0, 2.4), mats["wall"], 0.06),
        box("Gate_Right", (1.0, 1.15, 4.8), (2.35, 0, 2.4), mats["wall"], 0.06),
        box("Gate_Lintel", (5.7, 1.25, 0.72), (0, 0, 4.75), mats["stone"], 0.06),
        box("Gate_GoldTrim", (5.15, 0.10, 0.09), (0, -0.68, 4.88), mats["gold"], 0.015),
        box("Gate_TurqTrim", (4.82, 0.09, 0.07), (0, -0.69, 4.63), mats["turquoise"], 0.012),
        cylinder("Gate_Sun", 0.48, 0.11, (0, -0.70, 4.95), mats["gold"], 48, 0.018),
    ]
    for side in (-1, 1):
        for row in range(5):
            parts.append(
                curve_strip(
                    f"Gate_Wing_{side}_{row}",
                    [
                        (0.25 * side, -0.71, 4.95 - row * 0.12),
                        ((0.72 + row * 0.18) * side, -0.73, 5.08 - row * 0.10),
                        ((1.20 + row * 0.20) * side, -0.71, 4.96 - row * 0.08),
                    ],
                    0.04,
                    mats["gold"],
                )
            )
    obj = join_parts(parts, "SM_ARASH_Gate_A")
    add_asset_tag(obj)
    return obj


def build_materials() -> dict[str, bpy.types.Material]:
    return {
        "floor": make_polyhaven_material("M_Floor", "marble_01", (0.42, 0.34, 0.24, 1.0), roughness=0.58),
        "wall": make_polyhaven_material("M_Wall", "red_sandstone_wall", (0.28, 0.12, 0.055, 1.0), roughness=0.72),
        "stone": make_polyhaven_material("M_Stone", "sandstone_blocks_05", (0.30, 0.22, 0.14, 1.0), roughness=0.68),
        "metal": make_polyhaven_material("M_Metal", "metal_plate_02", (0.17, 0.10, 0.055, 1.0), metallic=0.85, roughness=0.28),
        "gold": make_principled("M_Gold", (0.68, 0.28, 0.035, 1.0), metallic=0.92, roughness=0.22),
        "turquoise": make_principled("M_Turquoise", (0.015, 0.30, 0.34, 1.0), metallic=0.28, roughness=0.36),
        "crimson": make_principled("M_Crimson", (0.23, 0.015, 0.018, 1.0), metallic=0.0, roughness=0.62),
        "emissive": make_principled(
            "M_Emissive",
            (0.55, 0.08, 0.005, 1.0),
            metallic=0.0,
            roughness=0.25,
            emission=(1.0, 0.10, 0.005, 1.0),
            emission_strength=12.0,
        ),
    }


def export_environment_kit(assets: list[bpy.types.Object]) -> None:
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    clear_selection()
    for obj in assets:
        obj.hide_set(False)
        obj.hide_render = False
        obj.select_set(True)

    bpy.context.view_layer.objects.active = assets[0]
    bpy.ops.export_scene.fbx(
        filepath=str(FBX_PATH),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y",
        axis_up="Z",
        add_leaf_bones=False,
        bake_anim=False,
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        path_mode="ABSOLUTE",
    )
    log(f"Exported {FBX_PATH}")


def main() -> None:
    reset_scene()
    materials = build_materials()
    assets = [
        create_floor_tile(materials),
        create_floor_medallion(materials),
        create_wall(materials),
        create_pillar(materials),
        create_brazier(materials),
        create_banner(materials),
        create_broken_column(materials),
        create_rubble(materials),
        create_gate(materials),
    ]

    for asset in assets:
        asset.location = (0.0, 0.0, 0.0)
        asset.rotation_euler = (0.0, 0.0, 0.0)
        asset.hide_set(False)
        asset.hide_render = False

    GENERATED_ROOT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    log(f"Saved {BLEND_PATH}")
    export_environment_kit(assets)
    log("Environment kit generation complete.")


if __name__ == "__main__":
    main()
