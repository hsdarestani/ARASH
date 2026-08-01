$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "ARASH.uproject"
$Editor = "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path $Editor)) {
    throw "Unreal Editor not found: $Editor"
}

if (-not (Test-Path $Project)) {
    throw "ARASH project not found: $Project"
}

$ExecCmds = @(
    "r.Streaming.LimitPoolSizeToVRAM 1",
    "r.Streaming.PoolSize 700",
    "r.Streaming.MipBias 1",
    "r.Streaming.MaxEffectiveScreenSize 1600",
    "r.TextureStreaming 1",
    "sg.TextureQuality 2",
    "sg.ShadowQuality 2",
    "sg.GlobalIlluminationQuality 2",
    "sg.ReflectionQuality 2",
    "sg.EffectsQuality 2",
    "r.ScreenPercentage 70"
) -join ","

Write-Host "[ARASH Dev Preview] Unreal Editor: $Editor"
Write-Host "[ARASH Dev Preview] Project: $Project"
Write-Host "[ARASH Dev Preview] Applying low-VRAM development settings."

& $Editor $Project "-ExecCmds=$ExecCmds"

if ($LASTEXITCODE -ne 0) {
    throw "Unreal Editor exited with code $LASTEXITCODE"
}
