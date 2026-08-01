$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "ARASH.uproject"
$Editor = "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$Map = "/Game/Maps/PrototypeArena/NewMap"

if (-not (Test-Path $Editor)) {
    throw "Unreal Editor not found: $Editor"
}

if (-not (Test-Path $Project)) {
    throw "ARASH project not found: $Project"
}

$ExecCmds = @(
    "r.Streaming.LimitPoolSizeToVRAM 1",
    "r.Streaming.PoolSize 560",
    "r.Streaming.MipBias 1.5",
    "r.Streaming.MaxEffectiveScreenSize 1400",
    "r.TextureStreaming 1",
    "r.Shadow.Virtual.Enable 0",
    "sg.TextureQuality 2",
    "sg.ShadowQuality 2",
    "sg.GlobalIlluminationQuality 2",
    "sg.ReflectionQuality 2",
    "sg.EffectsQuality 2",
    "r.ScreenPercentage 68"
) -join ","

Write-Host "[ARASH Dev Preview] Unreal Editor: $Editor"
Write-Host "[ARASH Dev Preview] Project: $Project"
Write-Host "[ARASH Dev Preview] Startup map: $Map"
Write-Host "[ARASH Dev Preview] Applying low-VRAM development settings."

$Arguments = @(
    $Project,
    $Map,
    "-ExecCmds=$ExecCmds"
)

$Process = Start-Process `
    -FilePath $Editor `
    -ArgumentList $Arguments `
    -WorkingDirectory $RepoRoot `
    -PassThru

Write-Host "[ARASH Dev Preview] Unreal Editor started successfully (PID $($Process.Id))."
