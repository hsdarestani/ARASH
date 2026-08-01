from __future__ import annotations

from pathlib import Path


HERE = Path(__file__).resolve().parent
SOURCE = HERE / "generate_arash_environment.py"


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")

    source = source.replace(
        '    scene.render.engine = "BLENDER_EEVEE_NEXT"',
        '    try:\n'
        '        scene.render.engine = "BLENDER_EEVEE"\n'
        '    except TypeError:\n'
        '        scene.render.engine = "BLENDER_EEVEE_NEXT"',
    )

    # Blender Principled BSDF has no Ambient Occlusion socket. AO stays in the
    # downloaded source texture for Unreal; Blender preview uses roughness/metallic.
    source = source.replace(
        '        links.new(separate.outputs["Red"], shader.inputs["Ambient Occlusion"])\n',
        '',
    )

    code = compile(source, str(SOURCE), "exec")
    namespace = {
        "__name__": "__main__",
        "__file__": str(SOURCE),
    }
    exec(code, namespace, namespace)


if __name__ == "__main__":
    main()
