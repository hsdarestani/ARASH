param(
    [string]$BlenderExe = "",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Generator = Join-Path $PSScriptRoot "generate_arash_hero.py"
$Output = Join-Path $RepoRoot "ArtSource\Generated\Blender\Character\ARASH_HeroKit.fbx"

if (-not (Test-Path $Generator)) {
    throw "Hero generator was not found: $Generator"
}

if ((Test-Path $Output) -and -not $Force) {
    Write-Host "[ARASH Hero] Reusing generated hero kit:"
    Write-Host "[ARASH Hero] $Output"
    exit 0
}

$Candidates = New-Object System.Collections.Generic.List[string]

if ($BlenderExe) {
    $Candidates.Add($BlenderExe)
}

$Command = Get-Command blender.exe -ErrorAction SilentlyContinue
if ($Command) {
    $Candidates.Add($Command.Source)
}

$SearchRoots = @(
    (Join-Path $env:ProgramFiles "Blender Foundation"),
    (Join-Path ${env:ProgramFiles(x86)} "Blender Foundation"),
    (Join-Path $env:LOCALAPPDATA "Programs\Blender Foundation"),
    "D:\Program Files\Blender Foundation",
    "D:\Blender",
    "E:\Program Files\Blender Foundation",
    "E:\Blender"
) | Where-Object { $_ -and (Test-Path $_) }

foreach ($Root in $SearchRoots) {
    Get-ChildItem $Root -Recurse -Filter blender.exe -File -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        ForEach-Object { $Candidates.Add($_.FullName) }
}

$ResolvedBlender = $Candidates |
    Where-Object { $_ -and (Test-Path $_) } |
    Select-Object -Unique |
    Select-Object -First 1

if (-not $ResolvedBlender) {
    throw @"
Blender was not found, so the prepared ARASH hero could not be generated.
Pass its full path explicitly, for example:
  -BlenderExe "C:\Program Files\Blender Foundation\Blender 4.5\blender.exe"
"@
}

Write-Host "[ARASH Hero] Blender: $ResolvedBlender"
Write-Host "[ARASH Hero] Generator: $Generator"

& $ResolvedBlender --background --python-exit-code 1 --python $Generator
if ($LASTEXITCODE -ne 0) {
    throw "Blender hero generation failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path $Output)) {
    throw "Hero generation finished but the FBX output was not found: $Output"
}

Write-Host "[ARASH Hero] Hero kit generated successfully:"
Write-Host "[ARASH Hero] $Output"
