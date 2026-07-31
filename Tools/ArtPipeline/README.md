# ARASH Art Pipeline

This folder bootstraps a high-fidelity art workflow without committing raw source downloads.

## Sources

The current bootstrap uses Poly Haven assets. The assets themselves are CC0. The live Poly Haven API is used by the fetch script and is credited by the script output/manifest as required by the API terms.

Selected starting set:

- `red_sandstone_wall` — weathered warm wall stone
- `sandstone_blocks_05` — large architectural blocks
- `marble_01` — light court floor / polished accents
- `metal_plate_02` — metal base suitable for bronze/gold art direction
- `dikhololo_sunset` — warm low-contrast sunset HDRI reference

ambientCG is also approved for future CC0 PBR additions.

## 1. Download source maps

From the repository root in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\ArtPipeline\fetch_polyhaven.ps1 -Resolution 4k
```

Downloads are placed under `ArtSource/PolyHaven/` and are intentionally ignored by Git.

Use `2k` while iterating on lower-end hardware; use `4k` for the normal production baseline. Reserve `8k` for hero assets where the camera actually justifies it.

## 2. Import and build Unreal materials

Close and reopen Unreal after pulling the project so `PythonScriptPlugin` and `EditorScriptingUtilities` are enabled.

**Important:** run the importer from the full Unreal Editor, not `UnrealEditor-Cmd` / `-run=pythonscript`. UE material editing can assert on rooted MaterialEditor objects when destructive material operations happen in commandlet mode.

Preferred flow:

1. Open `ARASH.uproject` normally.
2. Use `File -> Execute Python Script`.
3. Choose `Tools/ArtPipeline/import_polyhaven.py`.

Or run from Unreal's Python console:

```python
exec(open(r"D:/Projects/ARASH/Tools/ArtPipeline/import_polyhaven.py", encoding="utf-8").read())
```

The importer is intentionally non-destructive: it never clears existing material expression graphs. If a completed material already exists, it is left untouched. For a clean retry after a failed first import, remove or move only `/Game/Art/CC0/PolyHaven/` while the editor is closed, then rerun the importer.

The importer creates assets under:

`/Game/Art/CC0/PolyHaven/`

For textures it wires Base Color, DirectX Normal, AO, Roughness and Metallic. ARM maps are unpacked as R=AO, G=Roughness, B=Metallic. Displacement maps are imported but intentionally not connected yet; Nanite displacement will be introduced only for materials/meshes where it is visually justified.

After import completes, restart the editor so the ARASH GameMode constructor can resolve the newly-created court materials.

## 3. Version control

Raw `ArtSource/` files stay local/reproducible. Imported `.uasset` files should be committed through the project's existing Git LFS rules.

## Rendering target

PC is the visual reference platform: DX12 + Shader Model 6, Lumen GI/reflections, Virtual Shadow Maps and TSR. Mobile will use a separate scalable rendering path rather than forcing the PC art direction down to mobile limits.
