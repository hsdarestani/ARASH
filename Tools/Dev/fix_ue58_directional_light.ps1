$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Target = Join-Path $RepoRoot "Source\ARASH\Environment\ArashEnvironmentManager.cpp"

if (-not (Test-Path $Target)) {
    throw "Target source file was not found: $Target"
}

$Old = 'if (UDirectionalLightComponent* Light = It->GetDirectionalLightComponent())'
$New = 'if (UDirectionalLightComponent* Light = It->FindComponentByClass<UDirectionalLightComponent>())'

$Content = Get-Content -Path $Target -Raw

if ($Content.Contains($New)) {
    Write-Host "[ARASH] UE 5.8 directional-light compatibility fix is already applied."
    exit 0
}

if (-not $Content.Contains($Old)) {
    throw "Expected source pattern was not found. Refusing to modify the file."
}

$Updated = $Content.Replace($Old, $New)
Set-Content -Path $Target -Value $Updated -Encoding utf8NoBOM

$Verify = Get-Content -Path $Target -Raw
if (-not $Verify.Contains($New) -or $Verify.Contains($Old)) {
    throw "Source verification failed after patching."
}

Write-Host "[ARASH] Patched UE 5.8 directional-light component lookup:"
Write-Host $Target
