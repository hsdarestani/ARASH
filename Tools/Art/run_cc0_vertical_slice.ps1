param(
    [string]$UnrealRoot = "D:\UE_5.8",
    [string]$BlenderExe = "",
    [switch]$ForceDownload,
    [switch]$ForceHero,
    [switch]$SkipLaunch,
    [switch]$SkipBuild,
    [switch]$UseDx12
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Project = Join-Path $RepoRoot "ARASH.uproject"
$Installer = Join-Path $PSScriptRoot "install_cc0_vertical_slice.ps1"
$VisualTune = Join-Path $PSScriptRoot "apply_cc0_visual_tune.ps1"
$EnvironmentImporter = Join-Path $RepoRoot "Content\Python\import_cc0_vertical_slice.py"
$HeroGenerator = Join-Path $RepoRoot "Tools\Blender\run_hero.ps1"
$HeroIntegration = Join-Path $RepoRoot "Tools\Dev\integrate_generated_hero.ps1"
$HeroImporter = Join-Path $RepoRoot "Tools\ArtPipeline\import_blender_hero.py"
$HeroFbx = Join-Path $RepoRoot "ArtSource\Generated\Blender\Character\ARASH_HeroKit.fbx"
$BuildScript = Join-Path $UnrealRoot "Engine\Build\BatchFiles\Build.bat"
$EditorCmd = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$Editor = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$Map = "/Game/Maps/PrototypeArena/NewMap"

$RequiredPaths = @(
    $Project,
    $Installer,
    $VisualTune,
    $EnvironmentImporter,
    $HeroGenerator,
    $HeroIntegration,
    $HeroImporter,
    $BuildScript,
    $EditorCmd
)

foreach ($RequiredPath in $RequiredPaths) {
    if (-not (Test-Path $RequiredPath)) {
        throw "Required path not found: $RequiredPath"
    }
}

Write-Host "[ARASH Preview] Step 1/7 - Preparing pinned CC0 environment assets."
& $Installer -Force:$ForceDownload

Write-Host "[ARASH Preview] Step 2/7 - Applying Persian palette and low-VRAM runtime tune."
& $VisualTune

Write-Host "[ARASH Preview] Step 3/7 - Preparing and wiring the generated ARASH hero."
& $HeroIntegration

if ($ForceHero -or -not (Test-Path $HeroFbx)) {
    if ($BlenderExe) {
        & $HeroGenerator -BlenderExe $BlenderExe -Force:$ForceHero
    }
    else {
        & $HeroGenerator -Force:$ForceHero
    }
}
else {
    Write-Host "[ARASH Hero] Reusing generated hero kit: $HeroFbx"
}

if (-not (Test-Path $HeroFbx)) {
    throw "Prepared ARASH hero FBX was not created: $HeroFbx"
}

if ($SkipBuild) {
    Write-Host "[ARASH Preview] Step 4/7 - Win64 editor build skipped by request."
}
else {
    Write-Host "[ARASH Preview] Step 4/7 - Building ARASHEditor for Win64 Development."
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

    Write-Host "[ARASH Preview] Win64 editor build passed."
}

$CommonImportArguments = @(
    $Project,
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NoSound",
    "-NullRHI",
    "-SkipCompile"
)

Write-Host "[ARASH Preview] Step 5/7 - Importing and verifying environment assets."
$EnvironmentArguments = @($Project, "-ExecutePythonScript=$EnvironmentImporter") + $CommonImportArguments[1..($CommonImportArguments.Count - 1)]
& $EditorCmd @EnvironmentArguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal environment import failed with exit code $LASTEXITCODE"
}
Write-Host "[ARASH Preview] Environment import verification passed."

Write-Host "[ARASH Preview] Step 6/7 - Importing the prepared modular ARASH hero."
$HeroArguments = @($Project, "-ExecutePythonScript=$HeroImporter") + $CommonImportArguments[1..($CommonImportArguments.Count - 1)]
& $EditorCmd @HeroArguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal hero import failed with exit code $LASTEXITCODE"
}
Write-Host "[ARASH Preview] Hero import verification passed."

if ($SkipLaunch) {
    Write-Host "[ARASH Preview] Step 7/7 - Launch skipped by request."
    exit 0
}

if (-not (Test-Path $Editor)) {
    throw "Unreal Editor not found: $Editor"
}

Write-Host "[ARASH Preview] Step 7/7 - Opening the playable prototype arena."
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

Write-Host "[ARASH Preview] Unreal Editor started (PID $($Process.Id))."
Write-Host "[ARASH Preview] Press Play in PrototypeArena. The generated ARASH hero should replace the primitive placeholder."
