$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$Target = Join-Path $RepoRoot "Source\ARASH\Environment\ArashEnvironmentManager.cpp"

if (-not (Test-Path $Target)) {
    throw "Target source file was not found: $Target"
}

$Content = Get-Content -Path $Target -Raw
$Original = $Content

$Replacements = @(
    @('r.EyeAdaptationQuality 0', 'r.EyeAdaptationQuality 2'),
    @('Light->SetIntensity(3.2f);', 'Light->SetIntensity(4.0f);'),
    @('Light->SetLightColor(FLinearColor(1.0f, 0.58f, 0.36f));', 'Light->SetLightColor(FLinearColor(1.0f, 0.72f, 0.52f));'),
    @('Light->SetVolumetricScatteringIntensity(0.55f);', 'Light->SetVolumetricScatteringIntensity(0.35f);`r`n            Light->SetShadowAmount(0.68f);`r`n            Light->SetIndirectLightingIntensity(1.35f);'),
    @('Light->SetIntensity(0.42f);', 'Light->SetIntensity(1.15f);'),
    @('Light->SetIntensity(720.0f);', 'Light->SetIntensity(480.0f);'),
    @('Light->SetAttenuationRadius(360.0f);', 'Light->SetAttenuationRadius(300.0f);')
)

$Applied = 0
foreach ($Pair in $Replacements) {
    $Old = $Pair[0]
    $New = $Pair[1]

    if ($Content.Contains($Old)) {
        $Content = $Content.Replace($Old, $New)
        $Applied++
    }
}

$LoopOld = "for (TActorIterator<ADirectionalLight> It(World); It; ++It)`r`n    {`r`n        if (UDirectionalLightComponent* Light ="
$LoopNew = "for (TActorIterator<ADirectionalLight> It(World); It; ++It)`r`n    {`r`n        It->SetActorRotation(FRotator(-48.0f, -32.0f, 0.0f));`r`n`r`n        if (UDirectionalLightComponent* Light ="

if ($Content.Contains($LoopOld) -and -not $Content.Contains('It->SetActorRotation(FRotator(-48.0f, -32.0f, 0.0f));')) {
    $Content = $Content.Replace($LoopOld, $LoopNew)
    $Applied++
}

if ($Content -eq $Original) {
    if ($Content.Contains('r.EyeAdaptationQuality 2') -and $Content.Contains('Light->SetIntensity(1.15f);')) {
        Write-Host "[ARASH] Environment lighting v2 is already applied."
        exit 0
    }

    throw "No expected lighting patterns were found. Refusing to modify the source file."
}

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Target, $Content, $Utf8NoBom)

$Verify = Get-Content -Path $Target -Raw
$Required = @(
    'r.EyeAdaptationQuality 2',
    'Light->SetIntensity(4.0f);',
    'Light->SetIntensity(1.15f);',
    'Light->SetShadowAmount(0.68f);'
)

foreach ($Pattern in $Required) {
    if (-not $Verify.Contains($Pattern)) {
        throw "Lighting patch verification failed. Missing pattern: $Pattern"
    }
}

Write-Host "[ARASH] Applied environment lighting v2 ($Applied changes):"
Write-Host $Target
Write-Host "[ARASH] Eye adaptation restored, skylight raised, shadow density reduced, and sun angle normalized."
