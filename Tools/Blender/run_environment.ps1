$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Generator = Join-Path $PSScriptRoot "generate_arash_environment.py"

$Candidates = @()

$Command = Get-Command blender.exe -ErrorAction SilentlyContinue
if ($Command) {
    $Candidates += $Command.Source
}

$BlenderRoot = "C:\Program Files\Blender Foundation"
if (Test-Path $BlenderRoot) {
    $Candidates += Get-ChildItem $BlenderRoot -Recurse -Filter blender.exe -File -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        ForEach-Object { $_.FullName }
}

$BlenderExe = $Candidates | Select-Object -First 1
if (-not $BlenderExe) {
    throw "Blender was not found. Install Blender from blender.org, then run this script again."
}

Write-Host "[ARASH] Blender: $BlenderExe"
Write-Host "[ARASH] Generator: $Generator"
Write-Host "[ARASH] This may take several minutes on the first run."

& $BlenderExe --background --python $Generator
if ($LASTEXITCODE -ne 0) {
    throw "Blender environment generation failed with exit code $LASTEXITCODE"
}

$Output = Join-Path $RepoRoot "ArtSource\Generated\Blender\Environment\ARASH_EnvironmentKit.fbx"
if (-not (Test-Path $Output)) {
    throw "Generation finished but the FBX output was not found: $Output"
}

Write-Host "[ARASH] Environment kit generated successfully:"
Write-Host $Output
