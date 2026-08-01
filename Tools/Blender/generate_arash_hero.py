from __future__ import annotations

import math
from pathlib import Path

import bpy

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATED_ROOT = REPO_ROOT / "ArtSource" / "Generated" / "Blender"
EXPORT_DIR = GENERATED_ROOT / "Character"
FBX_PATH = EXPORT_DIR / "ARASH_HeroKit.fbx"
BLEND_PATH = GENERATED_ROOT / "ARASH_HeroKit.blend"


def log(message: str) -> None:
    print(f"[ARASH Hero] {message}", flush=True)


def reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for blocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
        for block in list(blocks):
            if block.users == 0:
                blocks.remove(block)


def material(name: str, color: tuple[float, float, float, float], metallic: float = 0.0, roughness: float = 0.55):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    return mat


def assign(obj, mat) -> None:
    if not obj.data.materials:
        obj.data.materials.append(mat)
    else:
        obj.data.materials[0] = mat


def apply_bevel(obj, width: float = 0.015, segments: int = 2) -> None:
    if width <= 0:
        return
    mod = obj.modifiers.new("ARASH_Bevel", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod.limit_method = "ANGLE"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)


def box(name, dims, loc, mat, bevel=0.015, rot=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cube_add(location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dims
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    apply_bevel(obj, bevel)
    assign(obj, mat)
    return obj


def cylinder(name, radius, depth, loc, mat, vertices=32, rot=(0.0, 0.0, 0.0), bevel=0.012):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    apply_bevel(obj, bevel)
    assign(obj, mat)
    return obj


def cone(name, r1, r2, depth, loc, mat, vertices=32, rot=(0.0, 0.0, 0.0), bevel=0.012):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2, depth=depth, location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    apply_bevel(obj, bevel)
    assign(obj, mat)
    return obj


def sphere(name, radius, loc, mat, scale=(1.0, 1.0, 1.0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=radius, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign(obj, mat)
    return obj


def torus(name, major, minor, loc, mat, rot=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=48, minor_segments=12, location=loc, rotation=rot)
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat)
    return obj


def curve_mesh(name: str, points: list[tuple[float, float, float]], bevel: float, mat):
    curve = bpy.data.curves.new(f"{name}_Curve", "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = bevel
    curve.bevel_resolution = 3
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for bp, co in zip(spline.bezier_points, points):
        bp.co = co
        bp.handle_left_type = "AUTO"
        bp.handle_right_type = "AUTO"
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    assign(obj, mat)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return bpy.context.object


def join_asset(parts, name: str, origin: tuple[float, float, float]):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.context.scene.cursor.location = origin
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    obj.location = (0.0, 0.0, 0.0)
    obj["ARASH_Asset"] = True
    return obj


def create_body(m):
    p = []
    for side in (-1, 1):
        y = side * 0.13
        p.append(cylinder(f"Leg_{side}", 0.085, 0.56, (0.0, y, -0.51), m["cloth_dark"], 24))
        p.append(box(f"Boot_{side}", (0.30, 0.18, 0.20), (-0.07, y, -0.80), m["leather"], 0.025))
        p.append(torus(f"BootGold_{side}", 0.09, 0.018, (0.0, y, -0.67), m["gold"]))
    p += [
        cone("Skirt", 0.34, 0.25, 0.46, (0.0, 0.0, -0.12), m["crimson"], 40),
        cone("Tunic", 0.30, 0.22, 0.68, (0.0, 0.0, 0.38), m["turquoise"], 40),
        box("ChestArmor", (0.26, 0.58, 0.48), (-0.055, 0.0, 0.42), m["leather"], 0.045),
        box("ChestPlate", (0.08, 0.39, 0.30), (-0.205, 0.0, 0.47), m["bronze"], 0.035),
        torus("Belt", 0.275, 0.035, (0.0, 0.0, 0.10), m["gold"]),
        box("BeltBuckle", (0.09, 0.12, 0.10), (-0.30, 0.0, 0.10), m["gold"], 0.02),
        box("Sash", (0.05, 0.18, 0.62), (-0.08, 0.22, -0.15), m["crimson"], 0.018, (0.0, math.radians(8), math.radians(-8))),
    ]
    for side in (-1, 1):
        p.append(sphere(f"Pauldron_{side}", 0.18, (0.0, side * 0.31, 0.60), m["bronze"], (0.80, 1.15, 0.55)))
        p.append(box(f"PauldronGold_{side}", (0.09, 0.34, 0.05), (-0.04, side * 0.30, 0.66), m["gold"], 0.018))
    return join_asset(p, "SM_ARASH_HeroBody_A", (0.0, 0.0, 0.0))


def create_head(m):
    origin = (0.0, 0.0, 0.80)
    p = [
        cylinder("Neck", 0.095, 0.18, (0.0, 0.0, 0.70), m["skin"], 24),
        sphere("Head", 0.18, (0.0, 0.0, 0.84), m["skin"], (0.92, 0.92, 1.10)),
        cone("Nose", 0.045, 0.0, 0.15, (-0.17, 0.0, 0.85), m["skin"], 20, (0.0, math.radians(-90), 0.0), 0.006),
        sphere("Hair", 0.19, (0.035, 0.0, 0.90), m["hair"], (0.92, 0.98, 0.72)),
        torus("Headband", 0.175, 0.018, (0.0, 0.0, 0.88), m["gold"]),
        cone("HelmetCrest", 0.07, 0.015, 0.26, (0.02, 0.0, 1.08), m["crimson"], 20),
    ]
    return join_asset(p, "SM_ARASH_HeroHead_A", origin)


def create_cape(m):
    origin = (0.10, 0.0, 0.58)
    verts = [
        (0.10, -0.31, 0.61), (0.10, 0.31, 0.61),
        (0.53, -0.44, -0.08), (0.53, 0.44, -0.08),
        (0.66, -0.26, -0.52), (0.66, 0.26, -0.52),
    ]
    faces = [(0, 1, 3, 2), (2, 3, 5, 4)]
    mesh = bpy.data.meshes.new("CapeMesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    cloth = bpy.data.objects.new("CapeCloth", mesh)
    bpy.context.collection.objects.link(cloth)
    assign(cloth, m["crimson"])
    solid = cloth.modifiers.new("CapeSolidify", "SOLIDIFY")
    solid.thickness = 0.018
    bpy.context.view_layer.objects.active = cloth
    bpy.ops.object.modifier_apply(modifier=solid.name)
    p = [
        cloth,
        torus("CapeClasp", 0.09, 0.025, (0.08, 0.0, 0.63), m["gold"], (math.radians(90), 0.0, 0.0)),
        box("CapeTrim", (0.04, 0.62, 0.05), (0.12, 0.0, 0.56), m["gold"], 0.012),
    ]
    return join_asset(p, "SM_ARASH_HeroCape_A", origin)


def create_arm(name: str, m):
    p = [
        cylinder("UpperArm", 0.085, 0.36, (0.18, 0.0, 0.0), m["turquoise"], 24, (0.0, math.radians(90), 0.0)),
        cylinder("Forearm", 0.073, 0.32, (0.50, 0.0, -0.015), m["skin"], 24, (0.0, math.radians(90), 0.0)),
        cylinder("Bracer", 0.095, 0.20, (0.39, 0.0, -0.01), m["leather"], 24, (0.0, math.radians(90), 0.0)),
        torus("BracerGold", 0.09, 0.014, (0.47, 0.0, -0.01), m["gold"], (0.0, math.radians(90), 0.0)),
        sphere("Hand", 0.083, (0.68, 0.0, -0.02), m["skin"], (1.1, 0.75, 0.75)),
    ]
    return join_asset(p, name, (0.0, 0.0, 0.0))


def create_bow_half(name: str, upper: bool, m):
    sign = 1.0 if upper else -1.0
    points = [(0.0, 0.0, 0.0), (0.0, -0.05, sign * 0.22), (0.0, -0.13, sign * 0.48), (0.0, -0.25, sign * 0.72)]
    wood = curve_mesh("BowWood", points, 0.035, m["wood"])
    tip = sphere("BowTip", 0.055, points[-1], m["gold"], (0.8, 0.8, 1.2))
    grip = cylinder("BowGrip", 0.045, 0.20, (0.0, 0.0, sign * 0.06), m["leather"], 24)
    return join_asset([wood, tip, grip], name, (0.0, 0.0, 0.0))


def create_bow_string(name: str, upper: bool, m):
    sign = 1.0 if upper else -1.0
    string = curve_mesh("String", [(0.0, 0.0, 0.0), (0.0, 0.0, sign * 0.72)], 0.008, m["string"])
    return join_asset([string], name, (0.0, 0.0, 0.0))


def create_quiver(m):
    p = [
        cylinder("QuiverBody", 0.12, 0.62, (0.0, 0.0, 0.0), m["leather"], 28),
        torus("QuiverRimA", 0.12, 0.018, (0.0, 0.0, 0.31), m["gold"]),
        torus("QuiverRimB", 0.12, 0.018, (0.0, 0.0, -0.31), m["gold"]),
    ]
    for index in range(4):
        angle = index * math.pi * 0.5
        p.append(cylinder(f"Arrow_{index}", 0.012, 0.72, (math.cos(angle) * 0.06, math.sin(angle) * 0.06, 0.20), m["wood"], 12))
        p.append(cone(f"ArrowHead_{index}", 0.03, 0.0, 0.10, (math.cos(angle) * 0.06, math.sin(angle) * 0.06, 0.61), m["gold"], 12))
    return join_asset(p, "SM_ARASH_HeroQuiver_A", (0.0, 0.0, 0.0))


def create_arrow(m):
    p = [
        cylinder("ArrowShaft", 0.014, 0.95, (0.0, 0.0, 0.0), m["wood"], 12, (0.0, math.radians(90), 0.0)),
        cone("ArrowHead", 0.055, 0.0, 0.16, (-0.55, 0.0, 0.0), m["gold"], 16, (0.0, math.radians(-90), 0.0)),
        box("FletchingA", (0.15, 0.06, 0.025), (0.46, 0.0, 0.0), m["crimson"], 0.005),
        box("FletchingB", (0.15, 0.025, 0.06), (0.46, 0.0, 0.0), m["crimson"], 0.005),
    ]
    return join_asset(p, "SM_ARASH_HeroArrow_A", (0.0, 0.0, 0.0))


def build_materials():
    return {
        "skin": material("Skin", (0.48, 0.24, 0.12, 1.0), 0.0, 0.60),
        "hair": material("Hair", (0.035, 0.018, 0.012, 1.0), 0.0, 0.72),
        "turquoise": material("Turquoise", (0.015, 0.20, 0.25, 1.0), 0.0, 0.55),
        "cloth_dark": material("ClothDark", (0.015, 0.05, 0.09, 1.0), 0.0, 0.68),
        "crimson": material("Crimson", (0.25, 0.01, 0.018, 1.0), 0.0, 0.62),
        "leather": material("Leather", (0.18, 0.055, 0.02, 1.0), 0.0, 0.58),
        "bronze": material("Bronze", (0.32, 0.12, 0.025, 1.0), 0.75, 0.30),
        "gold": material("Gold", (0.72, 0.32, 0.035, 1.0), 0.90, 0.22),
        "wood": material("Wood", (0.20, 0.065, 0.018, 1.0), 0.0, 0.50),
        "string": material("String", (0.72, 0.62, 0.42, 1.0), 0.0, 0.45),
    }


def export_assets(assets):
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    for asset in assets:
        asset.select_set(True)
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
        path_mode="AUTO",
    )
    log(f"Exported {FBX_PATH}")


def main() -> None:
    reset_scene()
    mats = build_materials()
    assets = [
        create_body(mats),
        create_head(mats),
        create_cape(mats),
        create_arm("SM_ARASH_HeroLeftArm_A", mats),
        create_arm("SM_ARASH_HeroRightArm_A", mats),
        create_bow_half("SM_ARASH_HeroBowUpper_A", True, mats),
        create_bow_half("SM_ARASH_HeroBowLower_A", False, mats),
        create_bow_string("SM_ARASH_HeroStringUpper_A", True, mats),
        create_bow_string("SM_ARASH_HeroStringLower_A", False, mats),
        create_quiver(mats),
        create_arrow(mats),
    ]

    GENERATED_ROOT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    log(f"Saved {BLEND_PATH}")
    export_assets(assets)
    log("Hero kit generation complete.")


if __name__ == "__main__":
    main()
