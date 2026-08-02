# CC0 art smoke test

This test replaces the generated Blender environment kit with a reproducible, pinned subset of **KayKit Dungeon Remastered** assets.

- Source: `KayKit-Game-Assets/KayKit-Dungeon-Remastered-1.0`
- Pinned commit: `b0ca9bd96a8072ab36a3a5464f00ed1e06a16d07`
- License: CC0 1.0
- Unreal destination: `/Game/Art/Generated/Environment`

The imported assets intentionally use the exact names already expected by `AArashEnvironmentManager`, so no map editing is required for the first smoke test.

## Run

From the repository root in Windows PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\Art\run_cc0_vertical_slice.ps1
```

The default Unreal installation is `D:\UE_5.8`. For another location:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\Art\run_cc0_vertical_slice.ps1 -UnrealRoot "E:\Epic\UE_5.8"
```

To download and import without opening the editor:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\Art\run_cc0_vertical_slice.ps1 -SkipLaunch
```

To discard the local download cache and fetch the pinned archive again:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\Art\run_cc0_vertical_slice.ps1 -ForceDownload
```

## What the command does

1. Downloads the pinned KayKit archive into `.art-cache/`.
2. Copies only the selected FBX files and their textures into ignored `ArtSource/` staging.
3. Starts `UnrealEditor-Cmd.exe` and runs `Content/Python/import_cc0_vertical_slice.py`.
4. Imports and saves nine static meshes, including the five meshes required by the current environment manager.
5. Verifies the required Unreal asset paths.
6. Opens `/Game/Maps/PrototypeArena/NewMap` with the existing low-VRAM preview settings.

## Expected result

Press **Play** in `PrototypeArena`. The runtime environment manager should replace the basic prototype geometry with:

- modular dungeon floor tiles
- a decorated center tile
- stone walls and columns
- lit torch props used as braziers
- banners, broken walls, rubble and a gated entrance

The existing ARASH camera, lighting and combat code remain unchanged.

## Troubleshooting

### The old blockout still appears

Check the Output Log for `[ARASH Environment] Generated Blender kit is incomplete`. At least one required imported asset is missing. Re-run the command without `-SkipLaunch` and inspect `[ARASH CC0 Import]` messages.

### Assets are tiny or rotated

The importer enables FBX scene and unit conversion. If one workstation still imports a different scale, adjust `import_uniform_scale` in `Content/Python/import_cc0_vertical_slice.py` and re-run; the process replaces existing assets.

### Download is blocked

The script downloads directly from the pinned public GitHub archive. A proxy, firewall or restricted PowerShell policy can block it. Downloading the same archive manually and placing it at `.art-cache\kaykit-dungeon-remastered\source.zip` lets the installer continue without another request.

## Repository policy

Downloaded source art and imported Unreal binaries are intentionally not committed by this change. The scripts, pinned source commit and license metadata make the smoke test reproducible while keeping the repository small.
