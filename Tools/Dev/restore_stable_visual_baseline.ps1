$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Set-Location $RepoRoot

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$BackupRoot = Join-Path $RepoRoot "Saved\LocalSourceBackup\$Timestamp"
$Files = @(
    "Source\ARASH\Player\ArashCharacter.cpp",
    "Source\ARASH\Environment\ArashEnvironmentManager.cpp"
)

New-Item -ItemType Directory -Force -Path $BackupRoot | Out-Null

foreach ($RelativePath in $Files) {
    $SourcePath = Join-Path $RepoRoot $RelativePath
    if (Test-Path $SourcePath) {
        $BackupPath = Join-Path $BackupRoot $RelativePath
        New-Item -ItemType Directory -Force -Path (Split-Path $BackupPath -Parent) | Out-Null
        Copy-Item $SourcePath $BackupPath -Force
        Write-Host "[ARASH Restore] Backed up $RelativePath"
    }
}

& git fetch origin main
if ($LASTEXITCODE -ne 0) {
    throw "git fetch origin main failed."
}

& git restore --source=origin/main -- @Files
if ($LASTEXITCODE -ne 0) {
    throw "git restore failed. Your backups are at: $BackupRoot"
}

$LaunchScript = Join-Path $RepoRoot "Tools\Dev\launch_arash_dev_preview.ps1"
$LaunchContent = @'
$ErrorActionPreference = "Stop"
$Editor = "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$Project = "D:\Projects\ARASH\ARASH.uproject"

if (-not (Test-Path $Editor)) { throw "Unreal Editor not found: $Editor" }
if (-not (Test-Path $Project)) { throw "Project not found: $Project" }

$ExecCmds = @(
    "r.Streaming.LimitPoolSizeToVRAM 1",
    "r.Streaming.PoolSize 700",
    "r.Streaming.MipBias 1",
    "r.Streaming.MaxEffectiveScreenSize 1600",
    "sg.TextureQuality 2",
    "sg.ShadowQuality 2",
    "sg.GlobalIlluminationQuality 2",
    "sg.ReflectionQuality 2",
    "sg.EffectsQuality 2",
    "r.ScreenPercentage 70"
) -join ","

& $Editor $Project -ExecCmds=$ExecCmds
'@

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($LaunchScript, $LaunchContent, $Utf8NoBom)

Write-Host ""
Write-Host "[ARASH Restore] Stable tracked source restored from origin/main."
Write-Host "[ARASH Restore] Local source backup: $BackupRoot"
Write-Host "[ARASH Restore] Low-VRAM editor launcher created: $LaunchScript"
Write-Host "[ARASH Restore] Rebuild before launching."
