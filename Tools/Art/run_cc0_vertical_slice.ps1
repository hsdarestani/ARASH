param(
    [string]$UnrealRoot = "D:\UE_5.8",
    [switch]$ForceDownload,
    [switch]$SkipLaunch,
    [switch]$SkipBuild,
    [switch]$UseDx12
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "ARASH.uproject"
$Installer = Join-Path $PSScriptRoot "install_cc0_vertical_slice.ps1"
$VisualTune = Join-Path $PSScriptRoot "apply_cc0_visual_tune.ps1"
$PythonScript = Join-Path $RepoRoot "Content\Python\import_cc0_vertical_slice.py"
$BuildScript = Join-Path $UnrealRoot "Engine\Build\BatchFiles\Build.bat"
$EditorCmd = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Editor = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$Map = "/Game/Maps/PrototypeArena/NewMap"

foreach ($RequiredPath in @($Project, $Installer, $VisualTune, $PythonScript, $BuildScript, $EditorCmd)) {
    if (-not (Test-Path $RequiredPath)) {
        throw "Required path not found: $RequiredPath"
    }
}

Write-Host "[ARASH CC0] Step 1/5 - Preparing pinned CC0 source assets."
& $Installer -Force:$ForceDownload

Write-Host "[ARASH CC0] Step 2/5 - Applying Persian palette and low-VRAM runtime tune."
& $VisualTune

if ($SkipBuild) {
    Write-Host "[ARASH CC0] Step 3/5 - Win64 editor build skipped by request."
}
else {
    Write-Host "[ARASH CC0] Step 3/5 - Building ARASHEditor for Win64 Development."
    $BuildArguments = @(
        "ARASHEditor",
        "Win64",
        "Development",
        "-Project=$Project",
        "-WaitMutex",
        "-NoHotReloadFromIDE"
    )

    & $BuildScript @BuildArguments
    if ($LASTEXITCODE -ne 0) {
        throw "ARASHEditor Win64 build failed with exit code $LASTEXITCODE"
    }

    Write-Host "[ARASH CC0] Win64 editor build passed."
}

Write-Host "[ARASH CC0] Step 4/5 - Importing and verifying assets in Unreal Engine."
$ImportArguments = @(
    $Project,
    "-ExecutePythonScript=$PythonScript",
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NoSound",
    "-NullRHI",
    "-SkipCompile"
)

& $EditorCmd @ImportArguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal CC0 import failed with exit code $LASTEXITCODE"
}

Write-Host "[ARASH CC0] Import verification passed."

if ($SkipLaunch) {
    Write-Host "[ARASH CC0] Step 5/5 - Launch skipped by request."
    exit 0
}

if (-not (Test-Path $Editor)) {
    throw "Unreal Editor not found: $Editor"
}

Write-Host "[ARASH CC0] Step 5/5 - Opening the playable prototype arena."
$ExecCmds = @(
    "r.Streaming.LimitPoolSizeToVRAM 1",
    "r.Streaming.PoolSize 320",
    "r.Streaming.MipBias 2.0",
    "r.Streaming.MaxEffectiveScreenSize 1024",
    "r.TextureStreaming 1",
    "r.Shadow.Virtual.Enable 0",
    "r.VolumetricFog 0",
    "r.DynamicGlobalIlluminationMethod 0",
    "r.ReflectionMethod 0",
    "r.Nanite 0",
    "sg.TextureQuality 1",
    "sg.ShadowQuality 1",
    "sg.GlobalIlluminationQuality 1",
    "sg.ReflectionQuality 1",
    "sg.EffectsQuality 1",
    "r.ScreenPercentage 60"
) -join ","

$LaunchArguments = @(
    $Project,
    $Map,
    "-SkipCompile",
    "-ExecCmds=$ExecCmds"
)

if (-not $UseDx12) {
    $LaunchArguments += "-d3d11"
}

$Process = Start-Process `
    -FilePath $Editor `
    -ArgumentList $LaunchArguments `
    -WorkingDirectory $RepoRoot `
    -PassThru

Write-Host "[ARASH CC0] Unreal Editor started (PID $($Process.Id))."
Write-Host "[ARASH CC0] Press Play in PrototypeArena to test the imported environment."
